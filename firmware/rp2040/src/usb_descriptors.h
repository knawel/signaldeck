// signaldeck: names shared by usb_descriptors.c, tusb_config.h and main.c.

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

// Interfaces, in the order they appear in the configuration descriptor.
enum {
    ITF_NUM_AUDIO_CONTROL = 0,  // UAC2: describes the audio function (clock, terminals)
    ITF_NUM_AUDIO_STREAMING,    // UAC2: carries the samples (alt 0 = idle, alt 1 = streaming)
    ITF_NUM_CDC,                // serial port for printf: control part…
    ITF_NUM_CDC_DATA,           // …and data part
    ITF_NUM_RESET,              // lets picotool reboot the board into BOOTSEL ("Run" in VS Code)
    ITF_NUM_TOTAL
};

// Audio "entities": the blocks of the UAC2 function, connected by these ID numbers.
//   USB stream ──► input terminal ──► output terminal (headphones)
//                        └── both run on the clock source
#define UAC2_ENTITY_CLOCK           0x04
#define UAC2_ENTITY_INPUT_TERMINAL  0x01
#define UAC2_ENTITY_OUTPUT_TERMINAL 0x03

// Endpoints. Bit 7 set = IN (device → host), clear = OUT (host → device).
#define EP_AUDIO_OUT    0x01    // isochronous: the samples
#define EP_AUDIO_FB     0x81    // isochronous: feedback, how fast we really play
#define EP_CDC_NOTIF    0x82
#define EP_CDC_OUT      0x03
#define EP_CDC_IN       0x83

// Length of the audio function's descriptors, from the IAD to the feedback endpoint.
// TinyUSB needs it as CFG_TUD_AUDIO_FUNC_1_DESC_LEN (tusb_config.h).
#define AUDIO_CS_AC_LEN (TUD_AUDIO_DESC_CLK_SRC_LEN      \
                       + TUD_AUDIO_DESC_INPUT_TERM_LEN   \
                       + TUD_AUDIO_DESC_OUTPUT_TERM_LEN)

#define AUDIO_DESC_LEN  (TUD_AUDIO_DESC_IAD_LEN          \
                       + TUD_AUDIO_DESC_STD_AC_LEN       \
                       + TUD_AUDIO_DESC_CS_AC_LEN        \
                       + AUDIO_CS_AC_LEN                 \
                       + TUD_AUDIO_DESC_STD_AS_INT_LEN   \
                       + TUD_AUDIO_DESC_STD_AS_INT_LEN   \
                       + TUD_AUDIO_DESC_CS_AS_INT_LEN    \
                       + TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN \
                       + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN \
                       + TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN \
                       + TUD_AUDIO_DESC_STD_AS_ISO_FB_EP_LEN)

#endif
