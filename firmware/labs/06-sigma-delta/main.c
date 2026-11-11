// Lab 6 — Sigma-delta DAC through PIO
//
// Goal: one pin and lab 2's RC filter again, but instead of PWM a 1-bit stream at 1.28 Mbit/s
// whose 1s are spread out as evenly as possible. A PIO state machine shifts the bits out at an
// exact rate; the CPU only computes them.
// Hardware: lab 2's circuit on GP14 (R1/C1 → FILT → GP26 ADC0, 47 µF → R2 → jack).
// See labs/labbook2.md, "Lab 6".

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "sigma_delta.pio.h"    // made from sigma_delta.pio by pioasm during the build

#define AUDIO_PIN       14      // the bit stream comes out here, into lab 2's filter
#define ADC_PIN         26      // FILT, measured by ADC input 0
#define ADC_INPUT       0

#define SD_PIO          pio0    // PIO block 0 of 2

#define SAMPLE_RATE_HZ  20000
#define OVERSAMPLE      64                                  // bits per audio sample
#define BIT_RATE_HZ     (SAMPLE_RATE_HZ * OVERSAMPLE)       // 1 280 000 bits per second
#define WORDS_PER_SAMPLE (OVERSAMPLE / 32)                  // the PIO takes 32 bits per word

#define TONE_HZ         440
#define TABLE_BITS      8
#define TABLE_SIZE      (1u << TABLE_BITS)

// Which part to run:
//   0 = Part A: the same 32-bit pattern over and over (tests the PIO on its own)
//   1 = Part B: steady levels through the sigma-delta modulator, measured with the ADC
//   2 = Part C: lab 3's sine through the modulator
#define PART            2

// Part B: after the measurements, hold 32768 + IDLE_OFFSET for 5 s and listen (task 6.5)
#define IDLE_OFFSET     64

// ADC: ADC_BURSTS bursts of ADC_BURST readings, 2 µs each. One burst (128 µs) is shorter than
// what the 8-word FIFO holds (8 × 25 µs = 200 µs), so the PIO never runs dry while we measure.
#define ADC_BURST       64
#define ADC_BURSTS      16

static uint sm;                 // our state machine (0 … 3) in SD_PIO

// --- the PIO side ---

static void sd_pio_init(void) {
    uint offset = pio_add_program(SD_PIO, &sigma_delta_program);   // copy the program into PIO memory
    sm = (uint)pio_claim_unused_sm(SD_PIO, true);
    pio_sm_config c = sigma_delta_program_get_default_config(offset);

    // TODO 2: (a) `out pins` writes 1 pin starting at AUDIO_PIN: sm_config_set_out_pins()
    //         (b) shift right (LSB first), autopull on, every 32 bits: sm_config_set_out_shift()
    //         (c) one instruction per bit: divider = clk_sys / BIT_RATE_HZ, sm_config_set_clkdiv()
    //             Print the divider with 5 decimals.
    sm_config_set_out_pins(&c, AUDIO_PIN, 1);
    sm_config_set_out_shift(&c, true, true, 32);
    float div = (float)clock_get_hz(clk_sys) / BIT_RATE_HZ;
    sm_config_set_clkdiv(&c, div);
    printf("%lu Hz, Div = %.5f, %lu bits/s\n", clock_get_hz(clk_sys), div, (uint32_t)BIT_RATE_HZ);


    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);                  // one 8-word TX FIFO, no RX

    pio_gpio_init(SD_PIO, AUDIO_PIN);                               // the pin now listens to PIO
    pio_sm_set_consecutive_pindirs(SD_PIO, sm, AUDIO_PIN, 1, true); // …as an output
    pio_sm_init(SD_PIO, sm, offset, &c);
    pio_sm_set_enabled(SD_PIO, sm, true);
}

// Did the state machine run out of bits since the last call? Reads and clears its TXSTALL flag.
static bool sd_stalled(void) {
    uint32_t mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
    bool stalled = (SD_PIO->fdebug & mask) != 0;
    SD_PIO->fdebug = mask;      // write 1 to clear
    return stalled;
}

#if PART > 0
// --- the modulator (Parts B and C) ---

static uint16_t sd_acc;         // the integrator: the error carried from one bit to the next

// The next 32 bits of the stream for level x (0 … 65535). Fraction of 1s = x / 65536.
static uint32_t sd_next_word(uint16_t x) {
    uint32_t word = 0;
    for (uint32_t n = 0; n < 32; n++) {
        // TODO 4: add x to sd_acc in 32 bits. Bit 16 of the sum (the carry) is this output bit.
        //         Keep the low 16 bits in sd_acc. Put the bit at position n of word.
        uint32_t sum = sd_acc + x;
        uint16_t bit = sum >> 16;
        sd_acc = sum;
        word |= bit << n;
    }
    return word;
}
#endif

#if PART < 2
// --- Parts A and B: steady signals, measured with the ADC ---

#if PART == 0
static uint32_t pattern;        // Part A: the word sent over and over
static uint32_t next_word(void) { return pattern; }
#else
static uint16_t level;          // Part B: the level fed to the modulator
static uint32_t next_word(void) { return sd_next_word(level); }
#endif

// Top up the TX FIFO without waiting.
static void feed(void) {
    while (!pio_sm_is_tx_fifo_full(SD_PIO, sm)) {
        pio_sm_put(SD_PIO, sm, next_word());
    }
}

static void hold_ms(uint32_t ms) {
    absolute_time_t end = make_timeout_time_ms(ms);
    while (!time_reached(end)) {
        feed();
    }
}

struct reading {
    float mean;                 // ADC counts, 0 … 4095
    uint16_t min, max;          // lowest and highest single reading: the ripple
    bool stalled;               // true = the PIO ran dry, so the numbers are wrong
};

static struct reading measure(void) {
    hold_ms(20);                // settle: 20 ms ≫ R1·C1 = 0.1 ms
    sd_stalled();               // clear the flag

    struct reading r = { 0.0f, 4095, 0, false };
    uint32_t sum = 0;
    for (int b = 0; b < ADC_BURSTS; b++) {
        feed();
        for (int n = 0; n < ADC_BURST; n++) {
            uint16_t v = adc_read();
            sum += v;
            if (v < r.min) r.min = v;
            if (v > r.max) r.max = v;
        }
    }
    r.mean = (float)sum / (ADC_BURSTS * ADC_BURST);
    r.stalled = sd_stalled();
    return r;
}

static void print_reading(uint32_t expected, struct reading r) {
    printf("expected %4lu  mean %7.1f  min %4u  max %4u  ripple %4u%s\n",
           expected, r.mean, r.min, r.max, r.max - r.min, r.stalled ? "  STALLED" : "");
}

#if PART == 0
static void part_a(void) {
    static const uint32_t patterns[] = {
        0x00000000, 0x11111111, 0x000000FF, 0x55555555, 0x0000FFFF, 0xFFFFFFFF,
    };
    for (uint32_t i = 0; i < sizeof patterns / sizeof patterns[0]; i++) {
        pattern = patterns[i];

        // TODO 3: count the 1s in `pattern` (bit k is (pattern >> k) & 1u).
        //         Expected ADC counts = ones × 4096 / 32.
        uint32_t ones = 0;
        for (uint16_t k = 0; k<32; k++){
            ones += (pattern >>k) & 1u; 
        }
        uint32_t expected = ones * 4096 / 32;
        

        printf("0x%08lx  %2lu/32  ", pattern, ones);
        print_reading(expected, measure());
    }
    pattern = 0x55555555;
    printf("holding 0x%08lx for 5 s: measure FILT with the multimeter\n\n", pattern);
    hold_ms(5000);
}
#else
static void part_b(void) {
    static const uint16_t levels[] = {
        0, 4096, 16384, 32768, 49152, 61440, 65535,     // across the range
        32768 + 16, 32768 + 64, 32768 + 256,            // finer than lab 2's 8-bit steps
    };
    for (uint32_t i = 0; i < sizeof levels / sizeof levels[0]; i++) {
        level = levels[i];
        // TODO 5: expected ADC counts for `level`. 65536 levels → 4096 counts: shift right by 4.
        uint32_t expected = level / 16;
        printf("level %5u  ", level);
        print_reading(expected, measure());
    }
    level = 32768 + IDLE_OFFSET;
    printf("holding level %u (32768 + %d) for 5 s: listen\n\n", level, IDLE_OFFSET);
    hold_ms(5000);
}
#endif

#else
// --- Part C: the sine synth from lab 3, now with 16-bit samples ---

static uint16_t sine_table[TABLE_SIZE];
static uint32_t phase;
static uint32_t phase_inc;

static void build_sine_table(void) {
    for (uint32_t n = 0; n < TABLE_SIZE; n++) {
        float angle = 2.0f * (float)M_PI * (float)n / TABLE_SIZE;
        sine_table[n] = (uint16_t)lroundf(32768.0f + 32767.0f * sinf(angle));
    }
}

static uint32_t phase_inc_for(uint32_t hz) {
    return (uint32_t)(((uint64_t)hz << 32) / SAMPLE_RATE_HZ);
}

static void part_c(void) {
    build_sine_table();
    phase_inc = phase_inc_for(TONE_HZ);
    printf("playing %d Hz, %d bits per sample\n", TONE_HZ, OVERSAMPLE);

    uint32_t stalls = 0;
    absolute_time_t next_print = make_timeout_time_ms(2000);
    sd_stalled();
    while (true) {
        // TODO 6: one sample: advance the phase, look up the table, then send WORDS_PER_SAMPLE
        //         words from sd_next_word() with pio_sm_put_blocking().
        phase += phase_inc;
        uint16_t x = sine_table[phase >> (32 - TABLE_BITS)];
        for (uint32_t w = 0; w < WORDS_PER_SAMPLE; w++) {
            pio_sm_put_blocking(SD_PIO, sm, sd_next_word(x));
        }

        if (sd_stalled()) stalls++;
        if (time_reached(next_print)) {
            printf("stalls so far: %lu\n", stalls);
            next_print = make_timeout_time_ms(2000);
        }
    }
}
#endif

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect

    printf("Lab 6: sigma-delta on GP%d, part %c\n", AUDIO_PIN, 'A' + PART);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_INPUT);

    sd_pio_init();

    while (true) {
#if PART == 0
        part_a();
#elif PART == 1
        part_b();
#else
        part_c();
#endif
    }
}
