#ifndef USB_AUDIO_H
#define USB_AUDIO_H

#include <stdint.h>
#include <stdbool.h>

// Call once before tusb_init()
void usb_audio_init(void);

// Call every iteration of the main loop — drains USB RX buffer into I2S
void usb_audio_task(void);

// Returns true when the host has selected the operational alternate setting
bool usb_audio_is_active(void);

#endif // USB_AUDIO_H
