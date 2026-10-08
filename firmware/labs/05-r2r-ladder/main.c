// Lab 5 — 8-bit R-2R ladder DAC
//
// Goal: build a DAC from 8 GPIOs and a ladder of resistors, measure how evenly spaced its
// 256 steps really are, then play lab 3's sine through it.
// Hardware: R-2R ladder on GP0–GP7 (zone C). OUT → GP27 (ADC1), and OUT → 47 µF → jack.
// See labs/labbook.md, "Lab 5".

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define DAC_BASE_PIN    0       // GP0 = bit 0 (LSB) … GP7 = bit 7 (MSB)
#define DAC_BITS        8
#define DAC_CODES       (1u << DAC_BITS)                        // 256
#define DAC_MASK        ((DAC_CODES - 1) << DAC_BASE_PIN)       // 0xFF << 0: one 1-bit per ladder pin

#define ADC_PIN         27      // ladder OUT, measured by ADC input 1
#define ADC_INPUT       1
#define ADC_SAMPLES     256     // readings averaged per measurement
#define ADC_PER_STEP    (4096.0f / DAC_CODES)                   // ideally 16 ADC counts per DAC step
#define SETTLE_US       50      // wait after changing the code before measuring

#define SAMPLE_RATE_HZ  20000   // one sample every 50 µs
#define TONE_HZ         440
#define TABLE_BITS      8
#define TABLE_SIZE      (1u << TABLE_BITS)

// Part A: 0 = measure the ladder: bit weights, then every one of the 256 codes
// Part B: 1 = play a sine through the ladder from a timer interrupt
#define PART_B          0

// 1 = write the 8 bits one at a time, BIT_DELAY_US apart, instead of all at once (task 5.4)
#define WRITE_SLOW      0
#define BIT_DELAY_US    1

// 1 = also print "code,adc" for every code, to paste into a spreadsheet (stretch)
#define PRINT_CSV       0

// Put `code` on the ladder: bit k of code drives GP(DAC_BASE_PIN + k).
static void dac_write(uint8_t code) {
#if !WRITE_SLOW
    // TODO 2: set all 8 ladder pins at once with gpio_put_masked(mask, value).
    //         The value has to be shifted to line up with DAC_BASE_PIN.
    (void)code;
#else
    // TODO 6: for k = 0 … DAC_BITS − 1: gpio_put() pin DAC_BASE_PIN + k to bit k of code,
    //         then busy_wait_us(BIT_DELAY_US).
    (void)code;
#endif
}

#if !PART_B
// --- Part A: measuring the ladder ---

static float measured[DAC_CODES];   // average ADC reading for each code, in ADC counts

// Average of ADC_SAMPLES readings, in ADC counts (0 … 4095). Same idea as lab 2's read_filt_mv().
static float read_adc_avg(void) {
    uint32_t sum = 0;
    for (int n = 0; n < ADC_SAMPLES; n++) {
        sum += adc_read();
    }
    return (float)sum / ADC_SAMPLES;
}

static float measure_code(uint8_t code) {
    dac_write(code);
    sleep_us(SETTLE_US);
    return read_adc_avg();
}

// Each bit alone: how many DAC steps is it really worth?
static void test_bit_weights(void) {
    float zero = measure_code(0);
    printf("bit  ideal  measured   (in DAC steps)\n");
    for (uint32_t k = 0; k < DAC_BITS; k++) {
        // TODO 3: measure code 1u << k, subtract `zero`, and divide by ADC_PER_STEP to get DAC steps.
        //         Print k, the ideal weight (1u << k) and the measured weight with 2 decimals.
        (void)k;
    }
    (void)zero;
}

// All 256 codes: is every step the same size (DNL), and is the staircase straight (INL)?
static void sweep_all_codes(void) {
    for (uint32_t c = 0; c < DAC_CODES; c++) {
        measured[c] = measure_code(c);
    }

    // The ideal line goes through the first and last measured points.
    float step = (measured[DAC_CODES - 1] - measured[0]) / (DAC_CODES - 1);
    printf("code 0: %.1f  code 255: %.1f  average step %.2f ADC counts (ideal %.2f)\n",
           measured[0], measured[DAC_CODES - 1], step, ADC_PER_STEP);

    float worst_dnl = 0.0f, worst_inl = 0.0f;
    uint32_t worst_dnl_code = 0, worst_inl_code = 0;
    bool monotonic = true;

    for (uint32_t c = 1; c < DAC_CODES; c++) {
        // TODO 4: DNL(c) = (measured[c] − measured[c − 1]) / step − 1
        //         INL(c) = (measured[c] − (measured[0] + c × step)) / step
        //         Keep the largest |DNL| and |INL| (use fabsf) with the code where each happened.
        //         If measured[c] ≤ measured[c − 1], the DAC is not monotonic.
    }

    printf("worst DNL %+.2f at code %lu, worst INL %+.2f at code %lu, %s\n",
           worst_dnl, worst_dnl_code, worst_inl, worst_inl_code,
           monotonic ? "monotonic" : "NOT monotonic");

    if (PRINT_CSV) {
        printf("code,adc\n");
        for (uint32_t c = 0; c < DAC_CODES; c++) {
            printf("%lu,%.2f\n", c, measured[c]);
        }
    }
}

#else
// --- Part B: the sine synth from lab 3, now with 8-bit DAC codes ---

static uint8_t sine_table[TABLE_SIZE];
static uint32_t phase;
static volatile uint32_t phase_inc;

static void build_sine_table(void) {
    for (uint32_t n = 0; n < TABLE_SIZE; n++) {
        float angle = 2.0f * (float)M_PI * (float)n / TABLE_SIZE;
        sine_table[n] = (uint8_t)lroundf(DAC_CODES / 2 + 127.0f * sinf(angle));
    }
}

static uint32_t phase_inc_for(uint32_t hz) {
    return (uint32_t)(((uint64_t)hz << 32) / SAMPLE_RATE_HZ);
}

static bool sample_callback(struct repeating_timer *t) {
    (void)t;
    // TODO 5: lab 3's callback, but write the sample to the ladder with dac_write().
    return true;
}
#endif

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect

    // TODO 1: set up the 8 ladder pins as outputs in one go:
    //         gpio_init_mask(DAC_MASK), then gpio_set_dir_out_masked(DAC_MASK).

    printf("Lab 5: R-2R ladder on GP%d-GP%d, mask 0x%08lx\n",
           DAC_BASE_PIN, DAC_BASE_PIN + DAC_BITS - 1, (uint32_t)DAC_MASK);

#if !PART_B
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_INPUT);

    while (true) {
        test_bit_weights();
        sweep_all_codes();

        dac_write(128);
        printf("holding code 128 for 5 s: measure OUT with the multimeter\n\n");
        sleep_ms(5000);
    }
#else
    build_sine_table();
    phase_inc = phase_inc_for(TONE_HZ);

    struct repeating_timer timer;
    add_repeating_timer_us(-(1000000 / SAMPLE_RATE_HZ), sample_callback, NULL, &timer);

    while (true) {
        printf("playing %d Hz through the ladder\n", TONE_HZ);
        sleep_ms(1000);
    }
#endif
}
