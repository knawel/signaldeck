// Lab 1 — 1-bit square-wave tone
//
// Goal: make a sound by switching one pin between 0 V and 3.3 V at an audio rate.
// See labs/labbook.md, "Lab 1", for wiring (series resistor + DC-blocking capacitor!) and hints.

#include <stdio.h>
#include "pico/stdlib.h"

#define AUDIO_PIN   14      // GP14 = audio left (see pin plan in the labbook)
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_DS5  622
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_G5   784
#define NOTE_A5   880
#define NOTE_B5   988
#define NOTE_C6   1047
#define NOTE_D6   1175
#define NOTE_E6   1319
#define REST      0      // silence

#define Q   400     // quarter note, ms: this sets the tempo
#define S   (Q / 4) // sixteenth note
#define E   (Q / 2) // eighth note

#define GAP   20      // ms of silence between notes

struct note {
    uint32_t hz;    // pitch (0 = rest)
    uint32_t ms;    // how long to play it
};

static const struct note melody[] = {
    // "Initializing..."
    { NOTE_C4, S }, { REST, S },
    { NOTE_C5, S }, { REST, S },
    { NOTE_G4, S }, { NOTE_G5, S },

    // Retro computer chirp
    { NOTE_C5, S }, { NOTE_D5, S },
    { NOTE_DS5, S }, { NOTE_G5, S },
    { NOTE_C6, E }, { REST, S },

    // "Game ready!"
    { NOTE_G4, S }, { NOTE_C5, S },
    { NOTE_E5, S }, { NOTE_G5, S },
    { NOTE_C6, Q },

    { REST, E },

    // Final PC-speaker sting
    { NOTE_C6, S },
    { NOTE_G5, S },
    { NOTE_C6, S },
    { NOTE_E6, Q + E },
};

#define MELODY_LEN (sizeof melody / sizeof melody[0])

static void play_note(uint32_t hz, uint32_t ms) {
    if (hz == 0) {
        sleep_ms(ms);
        return;
    }

    uint32_t half_period_us = 1000000 / (hz * 2);
    uint32_t cycles = ms * 1000 / (half_period_us * 2);

    for (uint32_t i = 0; i < cycles; ++i) {
        gpio_put(AUDIO_PIN, 1);
        sleep_us(half_period_us);
        gpio_put(AUDIO_PIN, 0);
        sleep_us(half_period_us);
    }
    sleep_us(GAP); // optional gap between notes
}

// Part A: 0 = toggle the pin from the main loop with sleep_us()
// Part B: 1 = let a hardware timer toggle the pin, main loop is free
#define USE_TIMER   0

#if USE_TIMER
// Called by the hardware timer every half period — this runs in an interrupt,
// so keep it tiny: no printf, no sleep.
// Return true to keep the timer repeating.
static bool toggle_callback(struct repeating_timer *t) {
    (void)t;    // parameter unused — this line tells the compiler that's intentional
    gpio_xor_mask(1u << AUDIO_PIN);
    return true;
}
#endif

int main(void) {
    stdio_init_all();

    // TODO 1: set up AUDIO_PIN as a digital output (same as lab 0).
    gpio_init(AUDIO_PIN);
    gpio_set_dir(AUDIO_PIN, GPIO_OUT);

    // TODO 2: compute the half period in microseconds from TONE_HZ.
    //         One period = one high half + one low half.
    uint32_t half_period_us = 0;
    half_period_us = 1000000 / (NOTE_A4 * 2);

    // TODO 3: print half_period_us AND the frequency you will actually get.
    //         Integer division throws away the remainder — is it exactly TONE_HZ?
    printf("half period: %lu us, frequency: %lu Hz\n", half_period_us, 1000000 / (half_period_us * 2));

#if USE_TIMER
    struct repeating_timer timer;

    // TODO 7: start a repeating timer that calls toggle_callback every half period.
    add_repeating_timer_us(-(int32_t)half_period_us, toggle_callback, NULL, &timer);
    while (true) {
        // TODO 8: the tone plays by itself now — prove the CPU is free by
        //         printing something once per second.
        printf("CPU is free\n");
        printf("tone freq: %lu Hz\n", 1000000 / (half_period_us * 2));
        sleep_ms(1000);
    }
#else
    while (true) {
        uint32_t note_number = 0; 
        while (note_number < MELODY_LEN) {
            play_note(melody[note_number].hz, melody[note_number].ms);
            note_number++;
        }
    }
#endif
}
