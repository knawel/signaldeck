// Lab 8 — USB audio → PCM5102A
//
// Goal: the board becomes a USB sound card. The computer sends samples over USB, we unpack
// them into I2S slots and lab 7's DMA + PIO play them through the PCM5102A. Then we solve the
// real problem of every USB DAC: the computer's clock and ours don't run at quite the same speed.
// Hardware: lab 7's wiring, unchanged (PCM5102A on GP0 BCK, GP1 LRCK, GP2 DIN).
// See labs/labbook2.md, "Lab 8".

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "tusb.h"               // TinyUSB: the USB stack
#include "usb_descriptors.h"
#include "i2s.pio.h"            // your lab 7 PIO program

// Which part to run:
//   0 = Part A: the computer finds a sound card. The samples it sends are thrown away.
//   1 = Part B: play them: USB → FIFO → DMA buffers → PIO → DAC, with a fixed feedback value
//   2 = Part C: the feedback follows the FIFO level, so the FIFO never runs dry or overflows
#define PART            0

// The fixed feedback value Parts B and C start from: 0 = nominal (TODO 6a), 1 = measured (TODO 6b)
#define FEEDBACK_START  0

// Digital attenuation: −6 dB per step. 4 = −24 dB. Earbuds on the module: keep it at 4 or more.
#define OUTPUT_SHIFT    4

#define PRINT_MS        1000    // status line this often

// --- I2S: lab 7, unchanged ---
#define I2S_BCK_PIN     0
#define I2S_LRCK_PIN    1
#define I2S_DIN_PIN     2
_Static_assert(I2S_LRCK_PIN == I2S_BCK_PIN + 1, "LRCK must be the pin right after BCK");

#define I2S_PIO         pio0
#define SLOT_BITS       32
#define BCK_HZ          (AUDIO_SAMPLE_RATE_HZ * SLOT_BITS * 2)     // 3 072 000
#define PIO_HZ          (BCK_HZ * 2)                               // 6 144 000

// --- sizes ---
#define BYTES_PER_SAMPLE    CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX
#define BYTES_PER_FRAME     (BYTES_PER_SAMPLE * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)
#define BUF_FRAMES          96      // per DMA buffer: 2 ms
#define FIFO_FRAMES         (CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / BYTES_PER_FRAME)  // TinyUSB's FIFO
#define TARGET_FRAMES       (FIFO_FRAMES / 2)   // start playing at half full, and try to stay there

// --- feedback: samples per 1 ms USB frame, in 16.16 fixed point (1.0 = 1 << 16) ---
// TODO 6: (a) the nominal value: exactly the number of samples per channel in one 1 ms frame
//             at AUDIO_SAMPLE_RATE_HZ. Write it with that name, not as a bare number.
//         (b) feedback_from_rate(), further down.
#define FEEDBACK_NOMINAL    0u
#define FEEDBACK_RANGE      (1u << 13)  // Part C may move ±1/8 sample per frame away from nominal

// Part C: feedback units (1/65536 sample per frame) per frame of FIFO error
#define KP                  8

static uint sm;

// =====================================================================================
// I2S and DMA: your lab 7 code
// =====================================================================================

static void i2s_init(float pio_hz) {
    uint offset = pio_add_program(I2S_PIO, &i2s_out_program);
    sm = (uint)pio_claim_unused_sm(I2S_PIO, true);
    pio_sm_config c = i2s_out_program_get_default_config(offset);

    sm_config_set_out_pins(&c, I2S_DIN_PIN, 1);
    sm_config_set_sideset_pins(&c, I2S_BCK_PIN);
    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_clkdiv(&c, clock_get_hz(clk_sys) / pio_hz);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    pio_gpio_init(I2S_PIO, I2S_BCK_PIN);
    pio_gpio_init(I2S_PIO, I2S_LRCK_PIN);
    pio_gpio_init(I2S_PIO, I2S_DIN_PIN);
    pio_sm_set_consecutive_pindirs(I2S_PIO, sm, I2S_BCK_PIN, 2, true);
    pio_sm_set_consecutive_pindirs(I2S_PIO, sm, I2S_DIN_PIN, 1, true);
    pio_sm_init(I2S_PIO, sm, offset, &c);
    pio_sm_set_enabled(I2S_PIO, sm, true);
}

// The sample rate we really play at, worked out from the divider the PIO really uses (lab 7).
static double actual_rate_hz(void) {
    uint32_t reg = I2S_PIO->sm[sm].clkdiv;
    double div = (reg >> PIO_SM0_CLKDIV_INT_LSB) + ((reg >> PIO_SM0_CLKDIV_FRAC_LSB) & 0xFFu) / 256.0;
    return clock_get_hz(clk_sys) / div / (2 * 2 * SLOT_BITS);
}

static uint32_t buffers[2][2 * BUF_FRAMES];     // L, R, L, R … ; static, so they start as silence
static int dma_chan[2];
static volatile int buffer_to_fill = -1;        // set by the DMA interrupt; −1 = nothing to do
static volatile uint32_t late_fills;            // lab 7's "underruns": we refilled too late

static void dma_handler(void) {
    for (int b = 0; b < 2; b++) {
        if (dma_channel_get_irq0_status(dma_chan[b])) {
            dma_channel_acknowledge_irq0(dma_chan[b]);
            dma_channel_set_read_addr(dma_chan[b], buffers[b], false);
            if (buffer_to_fill != -1) late_fills++;
            buffer_to_fill = b;
        }
    }
}

static void start_dma_audio(void) {
    dma_chan[0] = dma_claim_unused_channel(true);
    dma_chan[1] = dma_claim_unused_channel(true);

    for (int b = 0; b < 2; b++) {
        dma_channel_config c = dma_channel_get_default_config(dma_chan[b]);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        channel_config_set_dreq(&c, pio_get_dreq(I2S_PIO, sm, true));
        channel_config_set_chain_to(&c, dma_chan[1 - b]);
        dma_channel_configure(dma_chan[b], &c, &I2S_PIO->txf[sm], buffers[b], 2 * BUF_FRAMES, false);
        dma_channel_set_irq0_enabled(dma_chan[b], true);
    }

    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(dma_chan[0]);     // from now on the DAC always gets a clock, even in silence
}

// =====================================================================================
// Events: what the host asked for, printed later
// =====================================================================================
// TinyUSB calls our callbacks from inside tud_task(), and printf over USB calls tud_task()
// too. So a callback must never printf: it writes an event here, and the main loop prints it.

typedef enum { EV_MOUNT, EV_UNMOUNT, EV_SET_ALT, EV_GET, EV_SET } event_type_t;

typedef struct {
    event_type_t type;
    uint8_t entity, selector, request;  // which control (EV_GET, EV_SET)
    bool ok;                            // did we answer, or refuse (a "stall")?
    uint32_t value;                     // EV_SET_ALT: the alt setting; EV_SET: the value sent
} event_t;

#define EVENT_COUNT 32                  // a power of 2, so the indexes can wrap with a mask
static event_t events[EVENT_COUNT];
static uint32_t ev_written, ev_printed; // counters; index = counter & (EVENT_COUNT − 1)

static void log_event(event_type_t type, uint8_t entity, uint8_t selector, uint8_t request,
                      bool ok, uint32_t value) {
    if (ev_written - ev_printed == EVENT_COUNT) return;    // full: drop it
    events[ev_written & (EVENT_COUNT - 1)] = (event_t){type, entity, selector, request, ok, value};
    ev_written++;
}

static const char *entity_name(uint8_t id) {
    switch (id) {
        case UAC2_ENTITY_CLOCK:           return "clock";
        case UAC2_ENTITY_INPUT_TERMINAL:  return "input terminal";
        case UAC2_ENTITY_OUTPUT_TERMINAL: return "output terminal";
        default:                          return "entity ?";
    }
}

static void print_events(void) {
    while (ev_printed != ev_written) {
        event_t const *e = &events[ev_printed & (EVENT_COUNT - 1)];
        switch (e->type) {
            case EV_MOUNT:   printf("USB: configured by the host\n"); break;
            case EV_UNMOUNT: printf("USB: unplugged or reset\n"); break;
            case EV_SET_ALT:
                printf("USB: streaming interface -> alt %lu (%s)\n", e->value,
                       e->value ? "streaming" : "idle");
                break;
            case EV_GET:
            case EV_SET:
                printf("USB: %s %s, selector %u (%s), request %u (%s)",
                       e->type == EV_GET ? "GET" : "SET", entity_name(e->entity), e->selector,
                       e->entity == UAC2_ENTITY_CLOCK && e->selector == AUDIO_CS_CTRL_SAM_FREQ ? "sample rate"
                       : e->entity == UAC2_ENTITY_CLOCK && e->selector == AUDIO_CS_CTRL_CLK_VALID ? "clock valid"
                       : "?",
                       e->request,
                       e->request == AUDIO_CS_REQ_CUR ? "CUR" : e->request == AUDIO_CS_REQ_RANGE ? "RANGE" : "?");
                if (e->type == EV_SET) printf(" value %lu", e->value);
                printf(" -> %s\n", e->ok ? "answered" : "REFUSED");
                break;
        }
        ev_printed++;
    }
}

// =====================================================================================
// USB: TinyUSB calls these functions
// =====================================================================================

static bool streaming;          // the host has picked alt 1: samples are on their way
static uint32_t feedback;       // the value we send on the feedback endpoint (16.16)

void tud_mount_cb(void) {
    log_event(EV_MOUNT, 0, 0, 0, true, 0);
}

void tud_umount_cb(void) {
    streaming = false;
    log_event(EV_UNMOUNT, 0, 0, 0, true, 0);
}

// The host switched the streaming interface between alt 0 (idle) and alt 1 (streaming).
bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;
    uint8_t itf = tu_u16_low(p_request->wIndex);
    uint8_t alt = tu_u16_low(p_request->wValue);
    if (itf == ITF_NUM_AUDIO_STREAMING) {
        streaming = (alt != 0);
        if (streaming) tud_audio_fb_set(feedback);  // the feedback endpoint is open now: send a value
        log_event(EV_SET_ALT, 0, 0, 0, true, alt);
    }
    return true;
}

// We set the feedback value ourselves with tud_audio_fb_set(), so TinyUSB mustn't compute one.
void tud_audio_feedback_params_cb(uint8_t func_id, uint8_t alt_itf, audio_feedback_params_t *feedback_param) {
    (void)func_id;
    (void)alt_itf;
    feedback_param->method = AUDIO_FEEDBACK_METHOD_DISABLED;
    feedback_param->sample_freq = AUDIO_SAMPLE_RATE_HZ;
}

// The host asks the clock source about itself. Answer with
// tud_audio_buffer_and_schedule_control_xfer(), or return false to refuse ("stall").
static bool clock_get_request(uint8_t rhport, audio_control_request_t const *request) {
    tusb_control_request_t const *req = (tusb_control_request_t const *)request;

    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ && request->bRequest == AUDIO_CS_REQ_CUR) {
        // TODO 2a: the current sample rate, in Hz
        audio_control_cur_4_t rate = { .bCur = 0 };
        return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &rate, sizeof(rate));
    }
    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ && request->bRequest == AUDIO_CS_REQ_RANGE) {
        // TODO 2b: the list of rates we support: one "subrange" from bMin to bMax in steps of bRes.
        //          We support exactly one rate.
        audio_control_range_4_n_t(1) range = {
            .wNumSubRanges = 0,
            .subrange[0] = { .bMin = 0, .bMax = 0, .bRes = 0 },
        };
        return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &range, sizeof(range));
    }
    if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID && request->bRequest == AUDIO_CS_REQ_CUR) {
        // TODO 2c: is the clock running and stable? 1 = yes, 0 = no
        audio_control_cur_1_t valid = { .bCur = 0 };
        return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &valid, sizeof(valid));
    }
    return false;
}

// A GET request to one of the audio entities (clock, terminals).
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;
    bool ok = false;
    if (request->bEntityID == UAC2_ENTITY_CLOCK) ok = clock_get_request(rhport, request);
    log_event(EV_GET, request->bEntityID, request->bControlSelector, request->bRequest, ok, 0);
    return ok;
}

// A SET request. Our clock is fixed, so the only one we accept is "set the rate to 48 kHz".
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf) {
    (void)rhport;
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;
    uint32_t value = 0;
    bool ok = false;
    if (request->bEntityID == UAC2_ENTITY_CLOCK && request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ &&
        request->bRequest == AUDIO_CS_REQ_CUR && request->wLength == sizeof(audio_control_cur_4_t)) {
        value = (uint32_t)((audio_control_cur_4_t const *)buf)->bCur;
        ok = (value == AUDIO_SAMPLE_RATE_HZ);
    }
    log_event(EV_SET, request->bEntityID, request->bControlSelector, request->bRequest, ok, value);
    return ok;
}

// =====================================================================================
// Samples: from USB bytes to I2S slots
// =====================================================================================

// One sample on the wire: 3 bytes, least significant first. p points at the first one.
// Return it as a lab 7 slot: the 24-bit sample in bits 31…8, zeros in bits 7…0.
static inline uint32_t unpack_slot(const uint8_t *p) {
    // TODO 3
    (void)p;
    return 0;
}

// −6 dB per step of OUTPUT_SHIFT. Divides the *signed* sample (labbook, C toolbox).
static inline uint32_t attenuate(uint32_t slot) {
    return (uint32_t)((int32_t)slot / (1 << OUTPUT_SHIFT));
}

#if PART >= 1
// Fill one DMA buffer (BUF_FRAMES frames, L R L R …) from TinyUSB's FIFO.
// Returns how many frames really came from USB; the rest of the buffer must be silence.
static uint32_t fill_from_usb(uint32_t *buf) {
    uint8_t bytes[BUF_FRAMES * BYTES_PER_FRAME];
    // TODO 4: (a) read up to sizeof(bytes) bytes with tud_audio_read(); it returns how many it got
    //         (b) for each whole frame f that arrived: left in buf[2f], right in buf[2f + 1],
    //             each one unpack_slot() then attenuate(). Where in bytes[] does frame f start?
    //         (c) fill the rest of buf with silence (0)
    //         (d) return the number of whole frames
    (void)bytes;
    (void)buf;
    return 0;
}

// The FIFO level, averaged: 16.16 fixed point, in frames.
static uint32_t level_avg;

// TODO 5: an exponential moving average: move level_avg 1/64 of the way towards the new
//         level each call. Careful: the step can be negative.
static void update_level_avg(uint32_t level_frames) {
    (void)level_frames;
}

// TODO 6b: samples per 1 ms frame at rate_hz, in 16.16 fixed point, rounded.
static uint32_t feedback_from_rate(double rate_hz) {
    (void)rate_hz;
    return 0;
}
#endif

#if PART == 2
static uint32_t feedback_base;  // where the controller starts from

// TODO 7: a proportional controller. error = TARGET_FRAMES − the averaged level, in whole
//         frames (signed!). feedback = feedback_base + KP × error, kept within
//         FEEDBACK_NOMINAL ± FEEDBACK_RANGE.
//         Which way round? If the FIFO is too empty, we must ask the host for MORE samples.
static uint32_t feedback_control(void) {
    return feedback_base;
}
#endif

// =====================================================================================
// The main loop's audio job: refill a DMA buffer when one is free
// =====================================================================================

static bool playing;                    // we're taking samples out of the FIFO
static uint32_t short_fills, overflows;

static void service_audio(void) {
    int b = buffer_to_fill;             // read the volatile variable once
    if (b == -1) return;
    uint32_t *buf = buffers[b];

#if PART == 0
    for (int i = 0; i < 2 * BUF_FRAMES; i++) buf[i] = 0;   // Part A: silence, always
#else
    uint32_t level = tud_audio_available() / BYTES_PER_FRAME;

    if (!streaming) {
        playing = false;
    } else if (!playing && level >= TARGET_FRAMES) {
        playing = true;                 // half full: enough cushion to start
        level_avg = level << 16;
    }

    if (playing) {
        if (level > FIFO_FRAMES - BUF_FRAMES) overflows++;     // nearly full: packets get lost
        if (fill_from_usb(buf) < BUF_FRAMES) {
            short_fills++;              // ran dry: go silent until it's half full again
            playing = false;
        }
        update_level_avg(tud_audio_available() / BYTES_PER_FRAME);
#if PART == 2
        feedback = feedback_control();
        tud_audio_fb_set(feedback);
#endif
    } else {
        for (int i = 0; i < 2 * BUF_FRAMES; i++) buf[i] = 0;
    }
#endif
    buffer_to_fill = -1;
}

// =====================================================================================

static void print_banner(void) {
    printf("\nLab 8: USB audio, part %c. I2S on BCK GP%d, LRCK GP%d, DIN GP%d\n",
           'A' + PART, I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DIN_PIN);
    printf("playing at %.2f Hz; FIFO %d frames, target %d; output %d dB\n",
           actual_rate_hz(), FIFO_FRAMES, TARGET_FRAMES, -6 * OUTPUT_SHIFT);
    printf("feedback %lu = %.5f samples per frame\n", feedback, feedback / 65536.0);
}

static void print_status(void) {
#if PART == 0
    double level = (double)(tud_audio_available() / BYTES_PER_FRAME);
#else
    double level = level_avg / 65536.0;
#endif
    printf("%-9s FIFO %6.1f frames | feedback %.5f | short %lu, overflow %lu, late %lu\n",
           !streaming ? "idle" : playing ? "playing" : "buffering",
           level, feedback / 65536.0, short_fills, overflows, late_fills);
}

int main(void) {
    tusb_init();                // TinyUSB first: printf over USB rides on it
    stdio_init_all();
    i2s_init(PIO_HZ);
    start_dma_audio();

#if PART >= 1
    feedback = FEEDBACK_START ? feedback_from_rate(actual_rate_hz()) : FEEDBACK_NOMINAL;
#else
    feedback = FEEDBACK_NOMINAL;
#endif
#if PART == 2
    feedback_base = feedback;
#endif

    bool was_connected = false;
    absolute_time_t next_print = make_timeout_time_ms(PRINT_MS);
    while (true) {
        tud_task();             // let TinyUSB handle whatever the host sent: often!
        service_audio();

        bool connected = tud_cdc_connected();   // is a serial monitor listening?
        if (connected && !was_connected) print_banner();
        was_connected = connected;

        print_events();
        if (time_reached(next_print)) {
            print_status();
            next_print = make_timeout_time_ms(PRINT_MS);
        }
    }
}
