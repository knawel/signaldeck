#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_audio.h"
#include "i2s_audio.h"

int main(void) {
    board_init();
    stdio_init_all();

    printf("signaldeck boot\n");

    i2s_init();
    tusb_init();

    printf("USB audio ready\n");

    while (1) {
        tud_task();         // drive TinyUSB state machine
        usb_audio_task();   // drain USB RX buffer into I2S
    }
}
