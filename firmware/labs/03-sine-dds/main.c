// Lab 3 — Sine synth: lookup table + phase accumulator (DDS)
//
// Goal: play a clean sine at any frequency, accurate to a tiny fraction of a hertz,
// using a table of pre-computed sine values and one 32-bit addition per sample.
// Hardware: unchanged from lab 2 (R1/C1 filter on GP14, R2 → jack). See labs/labbook.md, "Lab 3".

#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

// musical constants
#define C4   262
#define D4   294
#define DS4  311     // Eb4
#define F4   349
#define G4   392
#define A4   440
#define AS4  466     // Bb4

#define C5   523
#define D5   587
#define DS5  622     // Eb5
#define F5   698
#define G5   784
#define A5   880
#define AS5  932     // Bb5
#define REST      0      // silence

#define Q    429
#define W    (Q * 4)
#define H    (Q * 2)
#define E    (Q / 2)
#define S    (Q / 4)

#define DQ   (Q + E)
#define DE   (E + S)

#define GAP   20      // ms of silence between notes

struct note {
    uint32_t hz;    // pitch (0 = rest)
    uint32_t ms;    // how long to play it
};


#define MELODY_LEN (sizeof melody / sizeof melody[0])

// end of musical constants


#define AUDIO_PIN       14      // GP14 = PWM slice 7, channel A
#define PWM_TOP         255     // 8-bit PWM, as in lab 2
#define PWM_LEVELS      (PWM_TOP + 1)

#define SAMPLE_RATE_HZ  20000   // one sample every 50 µs
#define TONE_HZ         440

// The sine table: TABLE_SIZE entries covering exactly one period of a sine.
#define TABLE_BITS      8
#define TABLE_SIZE      (1u << TABLE_BITS)      // 256

// Part A: 0 = steady TONE_HZ sine
// Part B: 1 = sweep from low to high frequency and listen past the Nyquist limit
#define PART_B          1

static uint16_t sine_table[TABLE_SIZE];

// The phase accumulator. A full uint32_t (0 … 2³² − 1) is one full period of the sine;
// adding phase_inc every sample moves around the circle, and overflow wraps back to 0 for free.
static uint32_t phase;

// How far to advance per sample. main() changes it while the timer callback reads it,
// so the compiler must not keep a stale copy in a register: that's what volatile means.
static volatile uint32_t phase_inc;

// Fill sine_table with one period of a sine, centred on the middle PWM level.
// Runs once at startup, so float and sinf() are fine here (never in the callback).
static void build_sine_table(void) {
    for (uint32_t n = 0; n < TABLE_SIZE; n++) {
        // TODO 1: angle = 2π · n / TABLE_SIZE, then scale sin(angle) from −1 … +1
        //         to PWM levels centred on PWM_LEVELS / 2. Round to the nearest integer.
        float angle = (float)n * 2.0f * (float)M_PI / TABLE_SIZE;
        float sine_value = sinf(angle);
        uint16_t half_pwm = PWM_LEVELS / 2;
        sine_table[n] = (uint16_t)(roundf((sine_value)*(half_pwm-1) + half_pwm));
    }

}

// Phase increment that produces hz at SAMPLE_RATE_HZ.
static uint32_t phase_inc_for(uint32_t hz) {
    // TODO 2: phase_inc = hz × 2³² / SAMPLE_RATE_HZ.
    //         2³² doesn't fit in 32 bits — what type do you need for the multiplication?
    uint32_t phase_inc = (uint32_t)(((uint64_t)hz << 32) / SAMPLE_RATE_HZ);
    return phase_inc;
}

// Called SAMPLE_RATE_HZ times a second: one sample. Keep it tiny — no printf, no float.
static bool sample_callback(struct repeating_timer *t) {
    (void)t;  // unused
    phase += phase_inc;
    uint32_t index = phase >> (32 - TABLE_BITS);
    pwm_set_gpio_level(AUDIO_PIN, sine_table[index]);

    // TODO 3: advance the phase, use its top TABLE_BITS bits as the table index,
    //         and send that table entry to the PWM.

    return true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect

    // TODO 4: PWM setup on AUDIO_PIN, exactly as in lab 2 (TOP = PWM_TOP, enabled).
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_set_wrap(slice_num, PWM_TOP);
    pwm_set_enabled(slice_num, true);

    build_sine_table();

    // TODO 5: print a few table entries (n = 0, 64, 128, 192) — do they match sin() at 0°, 90°, 180°, 270°?
    printf("Sine table entries:\n");
    for (uint32_t n = 0; n < TABLE_SIZE; n += TABLE_SIZE/4) {
        uint16_t angle_deg = (n * 360) / TABLE_SIZE;
        printf("a = %3u°, n = %3lu, sin table value: %u\n", angle_deg, n, sine_table[n]);
        }   

    phase_inc = phase_inc_for(TONE_HZ);

    // TODO 6: print phase_inc and the frequency you actually get: phase_inc × SAMPLE_RATE_HZ / 2³².
    //         Print it in milli-hertz (integers only) and compare with lab 2's 444.4 Hz.
    printf("Phase increment: %lu\n", phase_inc);
    uint32_t actual_hz_milli = (uint32_t)(((uint64_t)phase_inc * SAMPLE_RATE_HZ * 1000) >> 32);
    printf("Actual frequency: %lu mHz\n", actual_hz_milli);

    struct repeating_timer timer;
    add_repeating_timer_us(-(1000000 / SAMPLE_RATE_HZ), sample_callback, NULL, &timer);

#if PART_B
    while (true) {
        // TODO 7: sweep hz from 100 up to 19000 in steps of 100, spending ~50 ms on each.
        //         Update phase_inc and print hz every 1000 Hz so you know where you are.
        //         Listen carefully above SAMPLE_RATE_HZ / 2.
        while (true) {
            for (uint32_t hz = 100; hz <= 19000; hz += 100) {
                phase_inc = phase_inc_for(hz);
                if (hz % 250 == 0) {
                    printf("Frequency: %lu Hz\n", hz);
                }
                sleep_ms(250);
            }
            sleep_ms(1000);  // pause at the top before sweeping down
            phase_inc = phase_inc_for(440);
            sleep_ms(202);
        }
    }
    
    // Never return from main(): on the Pico SDK that halts the CPU, USB dies with it,
    // and picotool can no longer reboot the board into BOOTSEL for the next flash.

#else
    while (true) {
        tight_loop_contents();  // the timer does all the work
    }
#endif
}
