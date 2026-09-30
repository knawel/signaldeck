// Lab 2 — PWM DAC + RC low-pass filter
//
// Goal: turn a pin that can only be 0 V or 3.3 V into an output with 256 levels,
// by switching it very fast and averaging the result with a resistor and a capacitor.
// See labs/labbook.md, "Lab 2", for wiring (R1/C1 filter, R2 output resistor) and hints.

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"

#define AUDIO_PIN   14      // GP14 = PWM slice 7, channel A
#define ADC_PIN     26      // GP26 = ADC input 0, wired to the filter output (FILT)
#define ADC_INPUT   0

// The PWM counter counts 0, 1, … PWM_TOP, then wraps back to 0: PWM_TOP + 1 steps per period.
// The pin is high while counter < level, so level 0 … PWM_LEVELS gives 0 % … 100 % duty.
#define PWM_TOP     255
#define PWM_LEVELS  (PWM_TOP + 1)

#define VREF_MV     3300    // the GPIO high level and the ADC reference are both the 3V3 rail
#define ADC_SAMPLES 16      // readings averaged per measurement

// Part A: 0 = step through fixed DC levels and measure each one
// Part B: 1 = play a triangle-wave tone by changing the level SAMPLE_RATE_HZ times a second
#define PART_B      1

#define SAMPLE_RATE_HZ  20000
#define TONE_HZ         440
#define AMPLITUDE       PWM_LEVELS  // peak-to-peak, in PWM levels. Try smaller values (stretch).

// Measure the voltage on FILT in millivolts, averaged over ADC_SAMPLES readings.
static uint32_t read_filt_mv(void) {
    // TODO 4: add up ADC_SAMPLES calls to adc_read() (12-bit: 0 … 4095),
    //         then convert the sum to millivolts. Watch out for overflow and truncation.
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum += adc_read();
    }
    return (sum * VREF_MV) / (ADC_SAMPLES * 4096);

}

#if PART_B
static uint32_t samples_per_period;     // set in main() before the timer starts

// Called SAMPLE_RATE_HZ times a second from the timer interrupt: compute one sample
// and hand it to the PWM. Keep it tiny, as in lab 1: no printf, no sleep.
static bool sample_callback(struct repeating_timer *t) {
    (void)t;
    static uint32_t i = 0;  // position in the current period, 0 … samples_per_period − 1

    // TODO 6: first half of the period ramp the level up from 0 to AMPLITUDE,
    //         second half ramp it back down. Then advance i and wrap it at samples_per_period.
    uint32_t half = samples_per_period / 2;
    uint32_t level;
    if (i < half) {
        level = (i * AMPLITUDE) / half;
    } else {
        level = ((samples_per_period - i) * AMPLITUDE) / half;
    }
    pwm_set_gpio_level(AUDIO_PIN, level);
    
    i++;
    if (i >= samples_per_period) {
        i = 0;
    }
    return true;
}
#endif

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect, so the first prints aren't lost

    // TODO 1: hand AUDIO_PIN to the PWM peripheral, find out which slice it belongs to,
    //         set that slice's wrap value to PWM_TOP and enable it.
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_set_wrap(slice_num, PWM_TOP);
    pwm_set_enabled(slice_num, true);
    // pwm_set_clkdiv(slice_num, 10.0f); // Set clock divider to 1 for maximum frequency

    // TODO 2: print the system clock and the PWM frequency it gives you.
    printf("System clock: %lu Hz\n", clock_get_hz(clk_sys));
    printf("PWM frequency: %lu Hz\n", clock_get_hz(clk_sys) / (PWM_TOP + 1));

    // TODO 3: set up the ADC and route ADC_PIN into it.
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_INPUT);


#if PART_B
    samples_per_period = SAMPLE_RATE_HZ / TONE_HZ;

    // TODO 7: print samples_per_period and the tone frequency you will actually get.
    //         Integer division again — compare with lab 1.
    printf("Samples per period: %lu\n", samples_per_period);
    printf("Tone frequency: %lu Hz\n", SAMPLE_RATE_HZ / samples_per_period);


    struct repeating_timer timer;

    // TODO 8: start a repeating timer that calls sample_callback SAMPLE_RATE_HZ times a second.

    add_repeating_timer_us(-(1000000 / SAMPLE_RATE_HZ), sample_callback, NULL, &timer);

    while (true) {
        printf("FILT: %lu mV\n", read_filt_mv());
        sleep_ms(1000);
    }
#else
    // 0 %, 25 %, 50 %, 75 %, 100 % duty
    static const uint32_t levels[] = { 0, PWM_LEVELS / 4, PWM_LEVELS / 2, 3 * PWM_LEVELS / 4, PWM_LEVELS };

    while (true) {
        for (uint32_t k = 0; k < sizeof levels / sizeof levels[0]; k++) {
            // TODO 5: set the PWM level to levels[k], wait for FILT to settle,
            //         then print the level, the voltage you expect and the voltage the ADC measures.
            pwm_set_gpio_level(AUDIO_PIN, levels[k]);
            sleep_ms(2000);
            printf("Level: %lu, expected: %lu mV, measured: %lu mV\n", levels[k], (levels[k] * VREF_MV) / PWM_TOP
, read_filt_mv());

        }
    }
#endif
}
