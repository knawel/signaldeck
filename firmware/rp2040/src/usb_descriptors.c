#include "tusb.h"
#include "pico/unique_id.h"

// ---------------------------------------------------------------- interface numbers ---
enum {
    ITF_NUM_AUDIO_CONTROL = 0,
    ITF_NUM_AUDIO_STREAMING,
    ITF_NUM_TOTAL
};

// ---------------------------------------------------------------- audio entity IDs ---
#define CLKSRC_ID       1
#define INPUT_TERM_ID   2
#define OUTPUT_TERM_ID  3

// ---------------------------------------------------------------- endpoint addresses --
#define AUDIO_EP_OUT    0x01    // EP1 OUT (host -> device = playback)

// -------------------------------------------------------------- descriptor lengths ---
//
// UAC2 class-specific AC block: clock source + input terminal + output terminal
#define CS_AC_TOTAL_LEN  (TUD_AUDIO_DESC_CLK_SRC_LEN + \
                          TUD_AUDIO_DESC_INPUT_TERM_LEN + \
                          TUD_AUDIO_DESC_OUTPUT_TERM_LEN)

// Full audio portion (matches CFG_TUD_AUDIO_FUNC_1_DESC_LEN in tusb_config.h)
#define AUDIO_DESC_LEN  (TUD_AUDIO_DESC_IAD_LEN         + \
                         TUD_AUDIO_DESC_STD_AC_LEN       + \
                         TUD_AUDIO_DESC_CS_AC_LEN        + \
                         CS_AC_TOTAL_LEN                 + \
                         TUD_AUDIO_DESC_STD_AS_INT_LEN   + \
                         TUD_AUDIO_DESC_STD_AS_INT_LEN   + \
                         TUD_AUDIO_DESC_CS_AS_INT_LEN    + \
                         TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN + \
                         TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN + \
                         TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN)

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + AUDIO_DESC_LEN)

// ---------------------------------------------------------------- device descriptor --
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    // IAD requires Miscellaneous Device class at device level
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x6666,   // placeholder; replace with registered VID
    .idProduct          = 0x0001,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// --------------------------------------------------------------- config descriptor ---
uint8_t const desc_configuration[] = {
    // Standard configuration descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface Association: AudioControl + 1 AudioStreaming
    TUD_AUDIO_DESC_IAD(ITF_NUM_AUDIO_CONTROL, 2, 0),

    // Standard AudioControl interface (0 endpoints)
    TUD_AUDIO_DESC_STD_AC(ITF_NUM_AUDIO_CONTROL, 0, 0),

    // Class-specific AC header: UAC2, desktop speaker, CS-AC block length
    TUD_AUDIO_DESC_CS_AC(0x0200, AUDIO_FUNC_DESKTOP_SPEAKER, CS_AC_TOTAL_LEN,
                         AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS),

    // Clock source: internal fixed frequency
    TUD_AUDIO_DESC_CLK_SRC(CLKSRC_ID,
                            AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK,
                            (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS),
                            0x00, 0),

    // Input terminal: USB streaming -> 2 ch stereo
    TUD_AUDIO_DESC_INPUT_TERM(INPUT_TERM_ID, AUDIO_TERM_TYPE_USB_STREAMING,
                              0x00, CLKSRC_ID, 2,
                              AUDIO_CHANNEL_CONFIG_FRONT_LEFT |
                              AUDIO_CHANNEL_CONFIG_FRONT_RIGHT,
                              0, AUDIO_CTRL_NONE, 0),

    // Output terminal: headphones
    TUD_AUDIO_DESC_OUTPUT_TERM(OUTPUT_TERM_ID, AUDIO_TERM_TYPE_OUT_HEADPHONES,
                               0x00, INPUT_TERM_ID, CLKSRC_ID,
                               AUDIO_CTRL_NONE, 0),

    // AudioStreaming interface: alt 0 — zero bandwidth (required)
    TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING, 0, 0, 0),

    // AudioStreaming interface: alt 1 — operational (1 isochronous endpoint)
    TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING, 1, 1, 0),

    // Class-specific AS: terminal link, PCM format, stereo
    TUD_AUDIO_DESC_CS_AS_INT(INPUT_TERM_ID, AUDIO_CTRL_NONE,
                             AUDIO_FORMAT_TYPE_I, AUDIO_DATA_FORMAT_TYPE_I_PCM,
                             2,
                             AUDIO_CHANNEL_CONFIG_FRONT_LEFT |
                             AUDIO_CHANNEL_CONFIG_FRONT_RIGHT,
                             0),

    // Type I format: 3 bytes/subslot, 24-bit resolution
    TUD_AUDIO_DESC_TYPE_I_FORMAT(3, 24),

    // Isochronous OUT endpoint: async, 294 bytes max, 1ms interval
    TUD_AUDIO_DESC_STD_AS_ISO_EP(
        AUDIO_EP_OUT,
        (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA),
        CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX, 1),

    // Class-specific endpoint
    TUD_AUDIO_DESC_CS_AS_ISO_EP(
        AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK,
        AUDIO_CTRL_NONE,
        AUDIO_CS_AS_ISO_DATA_EP_LOCKDELAY_UNIT_UNDEFINED, 0x0000),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// ---------------------------------------------------------------- string descriptors -
static char serial_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},    // 0: English (0x0409)
    "Signaldeck",                   // 1: manufacturer
    "USB Headphone Interface",      // 2: product
    serial_str,                     // 3: serial (filled at runtime)
};

static uint16_t desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    if (index == 3 && serial_str[0] == '\0') {
        pico_unique_board_id_t uid;
        pico_get_unique_board_id(&uid);
        for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
            snprintf(serial_str + 2 * i, 3, "%02X", uid.id[i]);
        }
    }

    uint8_t chr_count;
    if (index == 0) {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
            return NULL;
        }
        const char *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) {
            desc_str[1 + i] = str[i];
        }
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return desc_str;
}
