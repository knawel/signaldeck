// Lab 7 — PCM5102A over I2S
//
// Goal: drive a real audio DAC chip. A PIO state machine makes the three I2S signals
// (BCK, LRCK, DIN); first we check them by reading our own pins back, then we play a
// 48 kHz / 24-bit stereo sine, and finally let DMA feed the PIO.
// Hardware: PCM5102A module on GP0 (BCK), GP1 (LRCK), GP2 (DIN); VIN from the 5V pin.
// See labs/labbook2.md, "Lab 7".

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "i2s.pio.h"            // made from i2s.pio by pioasm during the build

#define I2S_BCK_PIN     0       // bit clock
#define I2S_LRCK_PIN    1       // left/right clock: 0 = left, 1 = right
#define I2S_DIN_PIN     2       // the data

// Side-set drives BCK and LRCK as a pair of neighbouring pins. Stop the build if they aren't.
_Static_assert(I2S_LRCK_PIN == I2S_BCK_PIN + 1, "LRCK must be the pin right after BCK");

#define I2S_PIO         pio0

#define SAMPLE_RATE_HZ  48000
#define SLOT_BITS       32                                  // bits per channel in a frame
#define BCK_HZ          (SAMPLE_RATE_HZ * SLOT_BITS * 2)    // 3 072 000: 64 BCK periods per frame
#define PIO_HZ          (BCK_HZ * 2)                        // 6 144 000: 2 instructions per BCK period

// Which part to run:
//   0 = Part A: the PIO at a crawl, read back from our own pins (no DAC needed)
//   1 = Part B: a stereo sine; the CPU feeds the PIO
//   2 = Part C: the same sine; DMA feeds the PIO
#define PART            2

// Part A: 2000 instructions per second → BCK = 1 kHz, slow enough for the CPU to watch.
#define SLOW_PIO_HZ     2000
#define TEST_LEFT       ((uint32_t)0x80000001)  // first and last bit set
#define TEST_RIGHT      ((uint32_t)0x12345678)  // every hex digit different
#define CAPTURE_BITS    160             // BCK rising edges to record: 2.5 frames

// Parts B and C
#define LEFT_HZ         440
#define RIGHT_HZ        660
#define LEVEL_DB        -40             // dBFS. Earbuds in: never above −20. 0 = full scale = 2.1 V rms
#define FULL_SCALE      8388607         // largest 24-bit sample: 2^23 − 1
#define TABLE_BITS      10
#define TABLE_SIZE      (1u << TABLE_BITS)                  // 1024

// Part C: 256 frames at 48 kHz = 5.3 ms of audio per buffer
#define BUF_FRAMES      256

// Parts B and C print a status line this often. Task 7.7 makes it much shorter.
#define PRINT_MS        2000

static uint sm;                 // our state machine in I2S_PIO

// --- the PIO side ---

// Print what the clock divider really is, read back from the state machine's CLKDIV register.
static void print_rates(void) {
    uint32_t reg = I2S_PIO->sm[sm].clkdiv;
    uint32_t div_int = reg >> PIO_SM0_CLKDIV_INT_LSB;              // bits 31…16
    uint32_t div_frac = (reg >> PIO_SM0_CLKDIV_FRAC_LSB) & 0xFFu;  // bits 15…8, in 1/256ths
    double div = div_int + div_frac / 256.0;
    double pio_hz = clock_get_hz(clk_sys) / div;
    printf("clk_sys %lu Hz, divider %lu + %lu/256 = %.5f\n",
           clock_get_hz(clk_sys), div_int, div_frac, div);
    printf("PIO %.1f Hz -> BCK %.1f Hz -> sample rate %.2f Hz\n",
           pio_hz, pio_hz / 2, pio_hz / (2 * 2 * SLOT_BITS));
}

static void i2s_init(float pio_hz) {
    uint offset = pio_add_program(I2S_PIO, &i2s_out_program);
    sm = (uint)pio_claim_unused_sm(I2S_PIO, true);
    pio_sm_config c = i2s_out_program_get_default_config(offset);

    // TODO 2: (a) `out pins` writes 1 pin starting at I2S_DIN_PIN:     sm_config_set_out_pins()
    //         (b) side-set starts at I2S_BCK_PIN (LRCK is the next one): sm_config_set_sideset_pins()
    //         (c) shift LEFT (MSB first), autopull on, every 32 bits:  sm_config_set_out_shift()
    //         (d) divider = clk_sys / pio_hz:                           sm_config_set_clkdiv()
    sm_config_set_out_pins(&c, I2S_DIN_PIN, 1);
    sm_config_set_sideset_pins(&c, I2S_BCK_PIN);
    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_clkdiv(&c,  clock_get_hz(clk_sys)/ pio_hz);



    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);                     // one 8-word TX FIFO

    pio_gpio_init(I2S_PIO, I2S_BCK_PIN);                               // the pins now listen to PIO
    pio_gpio_init(I2S_PIO, I2S_LRCK_PIN);
    pio_gpio_init(I2S_PIO, I2S_DIN_PIN);
    pio_sm_set_consecutive_pindirs(I2S_PIO, sm, I2S_BCK_PIN, 2, true);  // BCK, LRCK: outputs
    pio_sm_set_consecutive_pindirs(I2S_PIO, sm, I2S_DIN_PIN, 1, true);  // DIN: output
    pio_sm_init(I2S_PIO, sm, offset, &c);
    pio_sm_set_enabled(I2S_PIO, sm, true);

    print_rates();
}

#if PART == 0
// --- Part A: a software logic analyser ---

static uint8_t cap_lrck[CAPTURE_BITS];  // LRCK at each BCK rising edge
static uint8_t cap_din[CAPTURE_BITS];   // DIN at each BCK rising edge: the bit the DAC reads

// Top up the TX FIFO without waiting: left, right, left, right …
static void feed_test_words(void) {
    static uint32_t count;      // static: keeps counting between calls, so the order never slips
    while (!pio_sm_is_tx_fifo_full(I2S_PIO, sm)) {
        pio_sm_put(I2S_PIO, sm, (count & 1u) ? TEST_RIGHT : TEST_LEFT);
        count++;
    }
}

// Watch our own pins and record LRCK and DIN at every rising edge of BCK, the moment a DAC
// would read them. Returns how many edges were recorded: CAPTURE_BITS, or fewer after 2 s.
static uint32_t capture(void) {
    uint32_t n = 0;
    bool bck_was = gpio_get(I2S_BCK_PIN);
    absolute_time_t give_up = make_timeout_time_ms(2000);
    while (n < CAPTURE_BITS && !time_reached(give_up)) {
        feed_test_words();
        uint32_t pins = gpio_get_all();             // all 30 input levels in one read
        bool bck = (pins >> I2S_BCK_PIN) & 1u;
        if (bck && !bck_was) {                      // low before, high now: a rising edge
            cap_lrck[n] = (pins >> I2S_LRCK_PIN) & 1u;
            cap_din[n] = (pins >> I2S_DIN_PIN) & 1u;
            n++;
        }
        bck_was = bck;
    }
    return n;
}

// Rebuild a 32-bit word from the 32 captured DIN bits starting at cap_din[start], MSB first.
static uint32_t read_word(uint32_t start) {
    uint32_t word = 0;
    // TODO 3: for k = 0 … 31, shift word left by one and put cap_din[start + k] into bit 0.
    word |= cap_din[start];
    for (uint16_t k = 1; k<SLOT_BITS; k++){
        word = word << 1u;
        word |= cap_din[start + k];
    }
    return word;
}

// One row of the picture: the bit at i0, then the left slot and the right slot in bytes.
static void print_row(const char *name, const uint8_t *bits, uint32_t i0) {
    printf("%s %u |", name, bits[i0]);
    for (uint32_t k = 1; k <= 2 * SLOT_BITS; k++) {
        printf("%u", bits[i0 + k]);
        if (k % 8 == 0) printf(k == SLOT_BITS ? " |" : " ");
    }
    printf("\n");
}

static void part_a(void) {
    uint32_t n = capture();
    if (n < CAPTURE_BITS) {
        printf("only %lu BCK rising edges in 2 s: is BCK toggling? (TODO 1 and 2)\n\n", n);
        return;
    }

    // A frame starts where LRCK falls from 1 (right) to 0 (left).
    uint32_t i0 = 0;
    for (uint32_t i = 1; i + 2 * SLOT_BITS < n; i++) {
        if (cap_lrck[i - 1] == 1 && cap_lrck[i] == 0) {
            i0 = i;
            break;
        }
    }
    if (i0 == 0) {
        printf("LRCK never fell from 1 to 0 in %lu edges: check the LRCK bits in TODO 1\n\n", n);
        return;
    }

    // How long does LRCK stay low, then high? I2S needs 32 and 32.
    uint32_t low = 0, high = 0;
    while (i0 + low < n && cap_lrck[i0 + low] == 0) low++;
    while (i0 + low + high < n && cap_lrck[i0 + low + high] == 1) high++;
    printf("LRCK low for %lu bits, then high for %lu bits (want 32 and 32)\n", low, high);

    printf("           |left slot                           |right slot\n");
    print_row("LRCK", cap_lrck, i0);
    print_row("DIN ", cap_din, i0);

    uint32_t left = read_word(i0 + 1);
    uint32_t right = read_word(i0 + 1 + SLOT_BITS);
    printf("left  0x%08lx  sent 0x%08lx  %s\n", left, TEST_LEFT, left == TEST_LEFT ? "OK" : "WRONG");
    printf("right 0x%08lx  sent 0x%08lx  %s\n\n", right, TEST_RIGHT, right == TEST_RIGHT ? "OK" : "WRONG");
}

#else
// --- Parts B and C: lab 3's sine synth, now signed 24-bit and in stereo ---

// Did the state machine run out of words since the last call? Reads and clears TXSTALL (lab 6).
static bool i2s_stalled(void) {
    uint32_t mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
    bool stalled = (I2S_PIO->fdebug & mask) != 0;
    I2S_PIO->fdebug = mask;     // write 1 to clear
    return stalled;
}

static int32_t sine_table[TABLE_SIZE];  // signed: −FULL_SCALE … +FULL_SCALE, already scaled by LEVEL_DB
static uint32_t phase_l, phase_r;
static uint32_t inc_l, inc_r;

static void build_sine_table(void) {
    // TODO 4: gain = 10^(LEVEL_DB / 20), with powf(). Watch out: LEVEL_DB is an int.
    //         Print LEVEL_DB, the gain (5 decimals) and the expected output, 2.1 V rms × gain.
    float gain = powf(10.0f, (float)LEVEL_DB / 20.0f);
    printf("level = %d, gain = %.05f, output = %.02f \n",LEVEL_DB, gain,  2.1*gain);

    for (uint32_t n = 0; n < TABLE_SIZE; n++) {
        float angle = 2.0f * (float)M_PI * (float)n / TABLE_SIZE;
        sine_table[n] = (int32_t)lroundf(gain * FULL_SCALE * sinf(angle));
    }
}

static uint32_t phase_inc_for(uint32_t hz) {
    return (uint32_t)(((uint64_t)hz << 32) / SAMPLE_RATE_HZ);
}

static void start_tones(void) {
    build_sine_table();
    inc_l = phase_inc_for(LEFT_HZ);
    inc_r = phase_inc_for(RIGHT_HZ);
    printf("playing %d Hz left, %d Hz right at %d dBFS\n", LEFT_HZ, RIGHT_HZ, LEVEL_DB);
}

// A signed 24-bit sample → one 32-bit I2S slot: the sample in bits 31…8, zeros in bits 7…0.
static inline uint32_t to_slot(int32_t sample) {
    // TODO 5: convert to uint32_t FIRST, then shift left by 8.
    uint32_t slot = sample;
    slot = slot <<8u;
    return slot;
}

// The next slot for one channel. `phase` points at that channel's phase accumulator.
static uint32_t next_slot(uint32_t *phase, uint32_t inc) {
    // TODO 6: advance the phase that `phase` points to by inc, look up the table with the
    //         top TABLE_BITS bits, and return the sample as a slot.
    *phase += inc;
    uint32_t index = *phase >> (32u - TABLE_BITS);
    return to_slot(sine_table[index]);
}
#endif

#if PART == 1
// --- Part B: the CPU feeds the PIO, as in lab 6 ---

static void part_b(void) {
    start_tones();
    uint32_t stalls = 0;
    absolute_time_t next_print = make_timeout_time_ms(PRINT_MS);
    i2s_stalled();
    while (true) {
        pio_sm_put_blocking(I2S_PIO, sm, next_slot(&phase_l, inc_l));   // left first…
        pio_sm_put_blocking(I2S_PIO, sm, next_slot(&phase_r, inc_r));   // …then right

        if (i2s_stalled()) stalls++;
        if (time_reached(next_print)) {
            printf("stalls so far: %lu\n", stalls);
            next_print = make_timeout_time_ms(PRINT_MS);
        }
    }
}

#elif PART == 2
// --- Part C: DMA feeds the PIO, two buffers taking turns (lab 4) ---

static uint32_t buffers[2][2 * BUF_FRAMES];     // L, R, L, R … : two words per frame
static int dma_chan[2];                         // DMA channel b plays buffers[b]
static volatile int buffer_to_fill = -1;        // set by the DMA interrupt; −1 = nothing to do
static volatile uint32_t underruns;

static void fill_buffer(uint32_t *buf) {
    // TODO 7: for each of the BUF_FRAMES frames f: the left slot goes in buf[2f],
    //         the right slot in buf[2f + 1]. Use next_slot().
    for (uint32_t frame=0; frame < BUF_FRAMES; frame++){
        buf[2*frame] = next_slot(&phase_l, inc_l);
        buf[2*frame+1] = next_slot(&phase_r, inc_r);
    }

}

// Lab 4's handler, unchanged: a channel has played its buffer and already chained to the other.
static void dma_handler(void) {
    for (int b = 0; b < 2; b++) {
        if (dma_channel_get_irq0_status(dma_chan[b])) {
            dma_channel_acknowledge_irq0(dma_chan[b]);
            dma_channel_set_read_addr(dma_chan[b], buffers[b], false);
            if (buffer_to_fill != -1) underruns++;
            buffer_to_fill = b;
        }
    }
}

static void start_dma_audio(void) {
    dma_chan[0] = dma_claim_unused_channel(true);
    dma_chan[1] = dma_claim_unused_channel(true);

    for (int b = 0; b < 2; b++) {
        dma_channel_config c = dma_channel_get_default_config(dma_chan[b]);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);    // one 32-bit slot per transfer
        channel_config_set_read_increment(&c, true);               // walk through the buffer…
        channel_config_set_write_increment(&c, false);             // …into one fixed FIFO register

        // TODO 8: (a) pace the channel with the PIO's TX DREQ: pio_get_dreq(I2S_PIO, sm, true),
        //         (b) chain it to the *other* channel,
        //         (c) configure it: write to &I2S_PIO->txf[sm], read from buffers[b],
        //             2 × BUF_FRAMES transfers, don't start yet.
        channel_config_set_dreq(&c, pio_get_dreq(I2S_PIO, sm, true));
        channel_config_set_chain_to(&c, dma_chan[1 - b]);
        dma_channel_configure(dma_chan[b], &c,
                      &I2S_PIO->txf[sm], 
                      buffers[b],
                      2 * BUF_FRAMES, false); 

        dma_channel_set_irq0_enabled(dma_chan[b], true);
    }

    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    fill_buffer(buffers[0]);
    fill_buffer(buffers[1]);
    dma_channel_start(dma_chan[0]);
}

static void part_c(void) {
    start_tones();
    start_dma_audio();
    uint32_t loops = 0, stalls = 0;
    absolute_time_t next_print = make_timeout_time_ms(PRINT_MS);
    i2s_stalled();
    while (true) {
        int b = buffer_to_fill;     // read the volatile variable once
        if (b != -1) {
            fill_buffer(buffers[b]);
            buffer_to_fill = -1;
        }
        if (i2s_stalled()) stalls++;
        loops++;
        if (time_reached(next_print)) {
            printf("stalls %lu, underruns %lu, main loop ran %lu times per print\n", stalls, underruns, loops);
            loops = 0;
            next_print = make_timeout_time_ms(PRINT_MS);
        }
    }
}
#endif

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect

    printf("Lab 7: I2S on BCK GP%d, LRCK GP%d, DIN GP%d, part %c\n",
           I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DIN_PIN, 'A' + PART);

#if PART == 0
    i2s_init(SLOW_PIO_HZ);
    while (true) {
        part_a();
        sleep_ms(3000);
    }
#else
    i2s_init(PIO_HZ);
#if PART == 1
    part_b();
#else
    part_c();
#endif
#endif
}
