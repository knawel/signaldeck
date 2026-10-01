// Lab 4 — Interrupt → DMA sample feeding
//
// Goal: measure how much CPU the lab 3 timer interrupt costs, then hand the sample feeding
// to the DMA engine: a hardware timer paces it, and the CPU only refills a buffer now and then.
// Hardware: unchanged from lab 2 (R1/C1 filter on GP14, R2 → jack). See labs/labbook.md, "Lab 4".

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"

#define AUDIO_PIN       14      // GP14 = PWM slice 7, channel A
#define PWM_TOP         255     // 8-bit PWM, as in lab 2
#define PWM_LEVELS      (PWM_TOP + 1)

#define SAMPLE_RATE_HZ  20000   // one sample every 50 µs
#define TONE_HZ         440

#define TABLE_BITS      8
#define TABLE_SIZE      (1u << TABLE_BITS)      // 256

// Samples per DMA buffer. 256 samples at 20 kHz = 12.8 ms of audio per buffer.
#define BUF_LEN         256

// Part A: 0 = lab 3's timer interrupt, one sample per interrupt
// Part B: 1 = DMA paced by a hardware timer, two buffers taking turns
#define PART_B          0

// 1 = block all interrupts for DISTURB_US every 10 ms, like a busy driver would
#define DISTURB         0
#define DISTURB_US      300

// --- the sine synth from lab 3 (your TODOs 1–3, finished) ---

static uint16_t sine_table[TABLE_SIZE];
static uint32_t phase;
static volatile uint32_t phase_inc;

static void build_sine_table(void) {
    for (uint32_t n = 0; n < TABLE_SIZE; n++) {
        float angle = 2.0f * (float)M_PI * (float)n / TABLE_SIZE;
        sine_table[n] = (uint16_t)lroundf(PWM_LEVELS / 2 + 127.0f * sinf(angle));
    }
}

static uint32_t phase_inc_for(uint32_t hz) {
    return (uint32_t)(((uint64_t)hz << 32) / SAMPLE_RATE_HZ);
}

// One step of the phase accumulator: advance, then look up the table.
static inline uint16_t next_sample(void) {
    phase += phase_inc;
    return sine_table[phase >> (32 - TABLE_BITS)];
}

static uint slice_num;                  // PWM slice that drives AUDIO_PIN
static volatile uint32_t underruns;     // times a buffer wasn't refilled in time (part B)

#if !PART_B
// --- Part A: one interrupt per sample ---

static bool sample_callback(struct repeating_timer *t) {
    (void)t;
    pwm_set_gpio_level(AUDIO_PIN, next_sample());
    return true;
}

static void service_audio(void) {
    // Nothing to do: the timer interrupt does all the work.
}

#else
// --- Part B: DMA with two buffers ---

static uint16_t buffers[2][BUF_LEN];        // buffers[0] and buffers[1], BUF_LEN samples each
static int dma_chan[2];                     // DMA channel b plays buffers[b]
static volatile int buffer_to_fill = -1;    // set by the DMA interrupt; -1 = nothing to do

// Write the next BUF_LEN samples into buf.
static void fill_buffer(uint16_t *buf) {
    // TODO 3: loop i = 0 … BUF_LEN − 1 and store next_sample() in buf[i].
    (void)buf;
}

// Runs when a DMA channel has played its whole buffer (and has already chained to the other one).
static void dma_handler(void) {
    for (int b = 0; b < 2; b++) {
        if (dma_channel_get_irq0_status(dma_chan[b])) {
            // TODO 6: (a) acknowledge the interrupt for dma_chan[b],
            //         (b) point the channel's read address back at buffers[b], without starting it,
            //         (c) if buffer_to_fill isn't −1, main() missed the last one: count an underrun,
            //         (d) set buffer_to_fill = b so main() refills it.
        }
    }
}

static void start_dma_audio(void) {
    // TODO 4: claim a DMA pacing timer and set its fraction to 1 / (clk_sys / SAMPLE_RATE_HZ).
    //         Print the denominator and clock_get_hz(clk_sys).
    int timer = 0;      // replace the 0 with the timer you claimed

    dma_chan[0] = dma_claim_unused_channel(true);
    dma_chan[1] = dma_claim_unused_channel(true);

    for (int b = 0; b < 2; b++) {
        dma_channel_config c = dma_channel_get_default_config(dma_chan[b]);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_16);    // one uint16_t per transfer
        channel_config_set_read_increment(&c, true);               // walk through the buffer…
        channel_config_set_write_increment(&c, false);             // …into one fixed PWM register

        // TODO 5: (a) pace the channel with the timer's DREQ,
        //         (b) chain it to the *other* channel,
        //         (c) configure it: write to &pwm_hw->slice[slice_num].cc, read from buffers[b],
        //             BUF_LEN transfers, don't start yet,
        //         (d) enable its completion interrupt on DMA_IRQ_0.
        (void)c;
        (void)timer;
    }

    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    fill_buffer(buffers[0]);
    fill_buffer(buffers[1]);
    dma_channel_start(dma_chan[0]);
}

static void service_audio(void) {
    // TODO 7: if buffer_to_fill isn't −1, fill that buffer, then set buffer_to_fill back to −1.
}
#endif

// Block every interrupt for DISTURB_US, once every 10 ms.
static void disturb(void) {
    static absolute_time_t next;    // static: keeps its value between calls
    if (time_reached(next)) {
        next = make_timeout_time_ms(10);
        uint32_t saved = save_and_disable_interrupts();
        busy_wait_us(DISTURB_US);
        restore_interrupts(saved);
    }
}

// How many times the main loop runs in one second. Fewer loops = less CPU left for main().
static uint32_t count_loops_1s(void) {
    uint32_t loops = 0;
    absolute_time_t end = make_timeout_time_ms(1000);
    // TODO 1: until `end` is reached: call service_audio(), call disturb() if DISTURB is 1,
    //         and count one loop.
    (void)end;
    return loops;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect

    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_set_wrap(slice_num, PWM_TOP);
    pwm_set_enabled(slice_num, true);

    build_sine_table();
    phase_inc = phase_inc_for(TONE_HZ);

    // Baseline: the same loop with no audio running.
    uint32_t idle_loops = count_loops_1s();
    printf("Idle: %lu loops/s\n", idle_loops);

#if PART_B
    start_dma_audio();
#else
    struct repeating_timer timer;
    add_repeating_timer_us(-(1000000 / SAMPLE_RATE_HZ), sample_callback, NULL, &timer);
#endif

    while (true) {
        uint32_t loops = count_loops_1s();

        // TODO 2: load in % = 100 − loops × 100 / idle_loops. Print loops, load and underruns.
        //         Does loops × 100 still fit in a uint32_t? Can the load come out negative?
        (void)loops;
    }
}
