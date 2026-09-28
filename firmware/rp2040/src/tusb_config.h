#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

// ------------------------------------------------------------------ target ---
#define CFG_TUSB_MCU    OPT_MCU_RP2040
#define CFG_TUSB_OS     OPT_OS_PICO
#define CFG_TUSB_DEBUG  0

// ------------------------------------------------------------------ device ---
#define CFG_TUD_ENABLED     1
#define CFG_TUD_MAX_SPEED   OPT_MODE_FULL_SPEED   // RP2040 is FS-only
#define CFG_TUD_ENDPOINT0_SIZE 64

// --------------------------------------------------------- class enable/disable
#define CFG_TUD_CDC    0
#define CFG_TUD_MSC    0
#define CFG_TUD_HID    0
#define CFG_TUD_MIDI   0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_AUDIO  1

// --------------------------------------------------- audio function settings ---
//
// Descriptor length of the audio portion of the config descriptor
// (IAD + AC interface + CS-AC header + clock + terminals + AS interfaces +
//  format type + isochronous endpoint descriptors).
// Recalculate whenever usb_descriptors.c changes.
//
// 8+9+9+8+17+12 + 9+9+16+6+7+8 = 118
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN   118

// One AudioStreaming interface (alternates: zero-bandwidth + operational)
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT   1

// Control buffer for SET_CUR / GET_CUR requests
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ  64

// -------------------------------------------------- playback (RX) settings ---
//
// "RX" in TinyUSB = data flowing from USB host into this device = playback.
//
// 24-bit stereo: 3 bytes/sample, 2 channels
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX  3
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX          2
#define CFG_TUD_AUDIO_FUNC_1_RX_FIFO_COUNT          1

// 48 kHz * 2 ch * 3 bytes = 288 bytes/ms; async allows +1 sample per frame
// 49 * 2 * 3 = 294 bytes worst-case per 1 ms USB frame
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX    (49 * 2 * 3)

// Software buffer: 4 ms headroom between USB fills and I2S drain
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ (CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX * 4)

// No TX (no microphone in V1)

#endif // TUSB_CONFIG_H
