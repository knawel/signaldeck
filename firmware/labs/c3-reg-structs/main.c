// Lab C3 — Register structs, volatile, and reading the disassembly
//
// Goal: write the same LED toggle four ways (raw address, your own struct, the SDK's struct,
// the SDK function) and prove in the .dis file that they compile to the same instructions.
// Then blink two LEDs like police lights, with one store per change.
// Then remove `volatile` from a button loop and watch the program hang.
// Hardware: same as lab C2 (LED on GP29, tactile switch from GP28 to GND), plus a second LED on GP8. See labs/c-labbook.md, "Lab C3".
// Datasheet: docs/RP-008371-DS-1-rp2040-datasheet.pdf

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/structs/sio.h"   // the SDK's sio_hw
#include "my_sio.h"                 // your MY_SIO

#define REG(addr)       (*(volatile uint32_t *)(addr))      // from lab C2
#define LED_PIN         29
#define BUTTON_PIN      28
#define LED2_PIN        8           // C3.3b: the second LED
#define LED_MASK        (1u << LED_PIN)
#define LED2_MASK       (1u << LED2_PIN)
#define BUTTON_MASK     (1u << BUTTON_PIN)

// Part A: 0 = blink four ways. Part B: 1 = wait for the button.
#define PART_B          0

// Part A only: 0 = the four toggles (C3.3), 1 = police lights on two LEDs (C3.3b)
#define POLICE          0

// Part B: 1 = read the button through a volatile pointer, 0 = through a plain one
#define USE_VOLATILE    1

#if !PART_B && !POLICE
// noipa: never inline these, merge them or rewrite them, so each one shows up in the .dis file by name.
__attribute__((noipa)) static void toggle_raw(void) {
    REG(0xD000001Cu) = LED_MASK;
}

__attribute__((noipa)) static void toggle_my_struct(void) {
    // TODO 3: the same write, through MY_SIO
}

__attribute__((noipa)) static void toggle_sdk_struct(void) {
    // TODO 4: the same write, through the SDK's sio_hw
}

__attribute__((noipa)) static void toggle_sdk_function(void) {
    // TODO 5: the same, with gpio_xor_mask()
}

#elif !PART_B
// One flash of every LED in `mask`: on, 40 ms, off, 60 ms.
__attribute__((noipa)) static void flash(uint32_t mask) {
    // TODO 7: switch the LEDs in mask on through MY_SIO, wait 40 ms, switch them off, wait 60 ms.
    //         Only the pins in mask may change: which two fields do that without a read?
    (void)mask;
}

// Both LEDs change state in ONE store.
__attribute__((noipa)) static void swap_one_store(void) {
    // TODO 8: flip GP29 and GP8 together through MY_SIO
}

// The same, with one store per LED.
__attribute__((noipa)) static void swap_two_stores(void) {
    // TODO 9: flip GP29, then GP8, as two separate writes
}

#else
// Wait until GP28 reads 0 (button pressed)
__attribute__((noipa)) static void wait_for_press(void) {
#if USE_VOLATILE
    volatile uint32_t *in = (volatile uint32_t *)0xD0000004u;
#else
    uint32_t *in = (uint32_t *)0xD0000004u;
#endif
    // TODO 6: loop while the BUTTON_MASK bit of *in is still 1. The loop body is empty: {}
    (void)in;
}
#endif

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect

    // Lab C2's TODOs 1, 2, 4, 5, now with SDK calls
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_pull_up(BUTTON_PIN);

    printf("=== Lab C3 ===\n");
    printf("SIO CPUID through MY_SIO: core %lu\n", MY_SIO->cpuid);
    printf("&MY_SIO->gpio_hi_in = %p\n", (void *)&MY_SIO->gpio_hi_in);
    printf("&sio_hw->gpio_hi_in = %p\n", (void *)&sio_hw->gpio_hi_in);

#if !PART_B && !POLICE
    // An array of function pointers: ways[i]() calls the i-th toggle. Each way gives 2 blinks.
    void (*ways[])(void) = { toggle_raw, toggle_my_struct, toggle_sdk_struct, toggle_sdk_function };
    const char *names[] = { "raw", "my struct", "sdk struct", "sdk function" };

    while (true) {
        for (int i = 0; i < 4; i++) {
            printf("%s\n", names[i]);
            for (int n = 0; n < 4; n++) {
                ways[i]();
                sleep_ms(150);
            }
            sleep_ms(600);
        }
        sleep_ms(1000);
    }
#elif !PART_B
    void (*swaps[])(void) = { swap_one_store, swap_two_stores };
    const char *names[] = { "one store", "two stores" };

    while (true) {
        printf("police\n");
        for (int n = 0; n < 4; n++) {
            flash(LED_MASK);
            flash(LED_MASK);
            sleep_ms(100);
            flash(LED2_MASK);
            flash(LED2_MASK);
            sleep_ms(100);
        }

        // TODO 10: start the alternating part with GP29 on and GP8 off
        for (int i = 0; i < 2; i++) {
            printf("alternate, %s\n", names[i]);
            for (int n = 0; n < 10; n++) {
                swaps[i]();
                sleep_ms(120);
            }
        }
        MY_SIO->gpio_clr = LED_MASK | LED2_MASK;
        sleep_ms(800);
    }
#else
    while (true) {
        printf("press the button...\n");
        wait_for_press();
        printf("pressed!\n");
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        while (!gpio_get(BUTTON_PIN)) {
            sleep_ms(10);   // wait for release
        }
        sleep_ms(20);       // let the contacts stop bouncing, or a bounce looks like a new press
    }
#endif
}
