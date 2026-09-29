// Lab 0 — hello, blink
//
// Goal: prove the whole chain works (edit → build → flash → run → see output)
// before touching any audio. See labs/labbook.md, "Lab 0", for wiring and hints.

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"

// External LED: GP29 → 330 Ω → LED anode, LED cathode → GND.
// (The onboard LED on GP16 is a WS2812 RGB LED and can't be driven with gpio_put.)
#define LED_PIN     29
#define BLINK_MS    56

int main(void) {
    stdio_init_all();   // sets up printf over USB serial

    // TODO 1: prepare LED_PIN as a digital output.
    //         Hint: two calls — one to init the pin, one to set its direction.

    uint32_t count = 0;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        // TODO 2: LED on, wait BLINK_MS, LED off, wait BLINK_MS.
        gpio_put(LED_PIN, 1);
        sleep_ms(BLINK_MS);
        gpio_put(LED_PIN, 0);
        sleep_ms(BLINK_MS*28);

        uint32_t time0 = time_us_32();
        printf("blink %lu\n", count);
        uint32_t time1 = time_us_32();
        printf("time: %lu\n us", time1 - time0);
        count++;
    }
}
