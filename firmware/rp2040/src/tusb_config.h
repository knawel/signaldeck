// signaldeck: TinyUSB settings. TinyUSB includes this file by name, so it must be called tusb_config.h.

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#include "usb_descriptors.h"

// --- the device ---
#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE     // the RP2040's one USB port is a device, not a host
#define CFG_TUD_ENDPOINT0_SIZE      64

// --- which classes (kinds of USB function) are in our device ---
#define CFG_TUD_AUDIO               1   // the sound card
#define CFG_TUD_CDC                 1   // the serial port printf uses
#define CFG_TUD_MSC                 0
#define CFG_TUD_HID                 0
#define CFG_TUD_MIDI                0
#define CFG_TUD_VENDOR              0   // the picotool reset interface brings its own driver

// --- serial port buffers ---
#define CFG_TUD_CDC_RX_BUFSIZE      64
#define CFG_TUD_CDC_TX_BUFSIZE      512
#define CFG_TUD_CDC_EP_BUFSIZE      64

// --- the audio function ---
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN       AUDIO_DESC_LEN  // see usb_descriptors.h
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT       1               // one streaming interface
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ    64              // room for one control request's data

#define CFG_TUD_AUDIO_ENABLE_EP_OUT         1               // samples come IN to us: an OUT endpoint
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX  2               // stereo
#define AUDIO_SAMPLE_RATE_HZ                48000           // the only rate we offer

// The stream format: 24-bit samples, each sent as 3 bytes. The biggest packet holds one audio
// frame more than the usual 48, because the feedback can ask the host for 49 now and then.
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX  3
#define CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX          24
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX \
    ((AUDIO_SAMPLE_RATE_HZ / 1000 + 1) * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)

// TinyUSB's receive FIFO: where packets wait until main.c reads them. 16 packets ≈ 16 ms.
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ   (16 * CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX)

// --- feedback: we tell the host how fast we really play (Parts B and C) ---
#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP                1
// Send the value in the 10.14 format that full-speed USB specifies, which macOS insists on.
// Windows wants 16.16 instead: set this to 0 there (see firmware/labs/labbook2.md, lab 8 Stretch).
#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_FORMAT_CORRECTION 1

#endif
