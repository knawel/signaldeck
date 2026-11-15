// signaldeck: the USB descriptors, the tables that tell the computer what this device is.
//
// When the board is plugged in, the computer asks for these one by one ("enumeration").
// TinyUSB answers by calling the tud_descriptor_..._cb() functions below.
// The TUD_..._DESCRIPTOR() macros come from TinyUSB (usbd.h): each one expands to a list of bytes.

#include <string.h>
#include "tusb.h"
#include "pico/unique_id.h"
#include "pico/usb_reset.h"
#include "usb_descriptors.h"

// --- device descriptor: the first thing the computer reads ---

// bcdDevice is the device version in "binary-coded decimal": each hex digit holds one decimal
// digit, laid out 0xJJMN = major JJ, minor M, patch N. So version 0.1.0 becomes 0x0010, and
// macOS System Information shows it as "Version: 0.10". The FW_VERSION_* numbers come from
// project(VERSION ...) in CMakeLists.txt, passed in as -D compiler flags.
//
// A minor or patch of 10 would not fit in one digit. _Static_assert (C11) checks a condition
// while compiling: if it is false the build stops with the message, so we cannot ship a wrong
// version number by accident.
_Static_assert(FW_VERSION_MAJOR <= 99 && FW_VERSION_MINOR <= 9 && FW_VERSION_PATCH <= 9,
               "bcdDevice holds major 0-99, minor 0-9, patch 0-9");
#define FW_BCD_DEVICE (((FW_VERSION_MAJOR / 10) << 12) | ((FW_VERSION_MAJOR % 10) << 8) | \
                       (FW_VERSION_MINOR << 4) | FW_VERSION_PATCH)

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,               // USB 2.0
    // Several functions (audio, serial) each grouped by an IAD: the device class must say so.
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCAFE,               // TinyUSB's test vendor ID: fine on your own desk only
    .idProduct          = 0x4008,               // kept from lab 8: the flash task passes --vid 0xcafe --pid 0x4008
    .bcdDevice          = FW_BCD_DEVICE,        // firmware version, see above
    .iManufacturer      = 1,                    // indexes into string_desc[] below
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// --- configuration descriptor: every interface and endpoint, in one long byte array ---

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + AUDIO_DESC_LEN + TUD_CDC_DESC_LEN + TUD_RPI_RESET_DESC_LEN)

static const uint8_t desc_configuration[] = {
    // Config number, interface count, string index, total length, attributes, max current in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // ===== The audio function (UAC2) =====
    // IAD: "the next 2 interfaces belong together"
    TUD_AUDIO_DESC_IAD(ITF_NUM_AUDIO_CONTROL, 2, 0),
    // AudioControl interface: no endpoints of its own, only the class-specific blocks below
    TUD_AUDIO_DESC_STD_AC(ITF_NUM_AUDIO_CONTROL, 0, 2),
    TUD_AUDIO_DESC_CS_AC(0x0200, AUDIO_FUNC_DESKTOP_SPEAKER, AUDIO_CS_AC_LEN, 0),
    // Clock source: an internal clock with a fixed rate. The host may read the rate and
    // whether the clock is valid (both "R", read-only), but can't change them.
    TUD_AUDIO_DESC_CLK_SRC(UAC2_ENTITY_CLOCK, AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK,
                           (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS) |
                           (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_VAL_POS),
                           0, 0),
    // Input terminal: where the samples enter the function, the USB stream, 2 channels
    TUD_AUDIO_DESC_INPUT_TERM(UAC2_ENTITY_INPUT_TERMINAL, AUDIO_TERM_TYPE_USB_STREAMING,
                              0, UAC2_ENTITY_CLOCK, CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX,
                              AUDIO_CHANNEL_CONFIG_FRONT_LEFT | AUDIO_CHANNEL_CONFIG_FRONT_RIGHT,
                              0, 0, 0),
    // Output terminal: where they leave it, headphones, fed straight from the input terminal
    TUD_AUDIO_DESC_OUTPUT_TERM(UAC2_ENTITY_OUTPUT_TERMINAL, AUDIO_TERM_TYPE_OUT_HEADPHONES,
                               0, UAC2_ENTITY_INPUT_TERMINAL, UAC2_ENTITY_CLOCK, 0, 0),

    // AudioStreaming interface, alternate setting 0: no endpoints, no bandwidth. The host
    // picks this one when nothing is playing.
    TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING, 0, 0, 0),
    // Alternate setting 1: streaming, with 2 endpoints (samples in, feedback out)
    TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING, 1, 2, 0),
    TUD_AUDIO_DESC_CS_AS_INT(UAC2_ENTITY_INPUT_TERMINAL, AUDIO_CTRL_NONE, AUDIO_FORMAT_TYPE_I,
                             AUDIO_DATA_FORMAT_TYPE_I_PCM, CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX,
                             AUDIO_CHANNEL_CONFIG_FRONT_LEFT | AUDIO_CHANNEL_CONFIG_FRONT_RIGHT, 0),
    // The sample format: 3-byte subslots, 24 bits used (tusb_config.h)
    TUD_AUDIO_DESC_TYPE_I_FORMAT(CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX,
                                 CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX),
    // The samples: isochronous, asynchronous (we run on our own clock), one packet per 1 ms frame.
    TUD_AUDIO_DESC_STD_AS_ISO_EP(EP_AUDIO_OUT,
                                 (uint8_t)(TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS |
                                           TUSB_ISO_EP_ATT_DATA),
                                 CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX, 1),
    TUD_AUDIO_DESC_CS_AS_ISO_EP(AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, AUDIO_CTRL_NONE,
                                AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, 0),
    // The feedback: 4 bytes, sent every 1 ms frame
    TUD_AUDIO_DESC_STD_AS_ISO_FB_EP(EP_AUDIO_FB, 4, 1),

    // ===== The serial port (CDC), for printf =====
    // Interface number, string index, notification endpoint + size, data out, data in, size
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EP_CDC_NOTIF, 8, EP_CDC_OUT, EP_CDC_IN, 64),

    // ===== The picotool reset interface =====
    TUD_RPI_RESET_DESCRIPTOR(ITF_NUM_RESET, 5),
};

// The compiler counts the bytes; stop the build if the hand-written total disagrees.
_Static_assert(sizeof(desc_configuration) == CONFIG_TOTAL_LEN, "CONFIG_TOTAL_LEN is wrong");

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;    // we only have one configuration
    return desc_configuration;
}

// --- string descriptors: the names the computer shows ---

static char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];   // filled from the flash chip's unique ID

static const char *string_desc[] = {
    NULL,                   // 0: the language list, handled separately below
    "signaldeck",           // 1: manufacturer
    "signaldeck USB DAC",   // 2: product: the name in your Sound settings
    serial,                 // 3: serial number
    "signaldeck serial",    // 4: the CDC interface
    "Reset",                // 5: the picotool reset interface
};

#define STRING_COUNT (sizeof(string_desc) / sizeof(string_desc[0]))

static uint16_t desc_str[32];   // USB strings are UTF-16: 2 bytes per character

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t chars;

    if (index == 0) {
        desc_str[1] = 0x0409;   // the only language we offer: English (US)
        chars = 1;
    } else {
        if (index >= STRING_COUNT) return NULL;
        if (index == 3) pico_get_unique_board_id_string(serial, sizeof(serial));
        const char *str = string_desc[index];
        chars = strlen(str);
        if (chars > 31) chars = 31;
        for (size_t i = 0; i < chars; i++) desc_str[1 + i] = (uint16_t)str[i];    // ASCII → UTF-16
    }

    // First element: the descriptor type in the high byte, the total length in bytes in the low byte
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chars + 2));
    return desc_str;
}
