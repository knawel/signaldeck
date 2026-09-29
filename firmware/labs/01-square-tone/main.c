// Lab 1 — 1-bit square-wave tone
//
// Goal: make a sound by switching one pin between 0 V and 3.3 V at an audio rate.
// See labs/labbook.md, "Lab 1", for wiring (series resistor + DC-blocking capacitor!) and hints.

#include <stdio.h>
#include "pico/stdlib.h"

#define AUDIO_PIN   14      // GP14 = audio left (see pin plan in the labbook)
#define TONE_HZ     440     // A4, the orchestra tuning note

// Part A: 0 = toggle the pin from the main loop with sleep_us()
// Part B: 1 = let a hardware timer toggle the pin, main loop is free
#define USE_TIMER   0

#if USE_TIMER
// Called by the hardware timer every half period — this runs in an interrupt,
// so keep it tiny: no printf, no sleep.
// Return true to keep the timer repeating.
static bool toggle_callback(struct repeating_timer *t) {
    (void)t;    // parameter unused — this line tells the compiler that's intentional

    // TODO 6: flip AUDIO_PIN (high → low, low → high).

    return true;
}
#endif

int main(void) {
    stdio_init_all();

    // TODO 1: set up AUDIO_PIN as a digital output (same as lab 0).

    // TODO 2: compute the half period in microseconds from TONE_HZ.
    //         One period = one high half + one low half.
    uint32_t half_period_us = 0;

    // TODO 3: print half_period_us AND the frequency you will actually get.
    //         Integer division throws away the remainder — is it exactly TONE_HZ?

#if USE_TIMER
    struct repeating_timer timer;

    // TODO 7: start a repeating timer that calls toggle_callback every half period.

    while (true) {
        // TODO 8: the tone plays by itself now — prove the CPU is free by
        //         printing something once per second.
    }
#else
    while (true) {
        // TODO 4: pin high, wait half period, pin low, wait half period.
    }
#endif
}
