#include "usb_audio.h"
#include "i2s_audio.h"
#include "tusb.h"
#include <string.h>

static bool s_active = false;

void usb_audio_init(void) {
    s_active = false;
}

void usb_audio_task(void) {
    if (!s_active) return;

    // How many bytes are waiting in TinyUSB's RX FIFO?
    uint32_t available = tud_audio_available();
    if (available == 0) return;

    // Read one endpoint packet worth at a time
    uint8_t buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX];
    uint32_t bytes_read = tud_audio_read(buf, sizeof(buf));
    if (bytes_read == 0) return;

    // Hand samples to I2S — the I2S module owns the DMA/PIO path
    i2s_write(buf, bytes_read);
}

bool usb_audio_is_active(void) {
    return s_active;
}

// ---------------------------------------------------------------- TinyUSB callbacks --

// Called when the host selects an alternate setting on the AS interface
void tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;
    uint8_t const itf  = (uint8_t)p_request->wIndex;
    uint8_t const alt  = (uint8_t)p_request->wValue;
    (void)itf;

    if (alt == 1) {
        // Operational: host is about to stream audio
        s_active = true;
    } else {
        // Zero-bandwidth: host stopped streaming
        s_active = false;
        i2s_mute();
    }
}

// Called after audio data arrives from the host into the RX FIFO.
// We use polling in usb_audio_task() instead, so this is a no-op.
bool tud_audio_rx_done_pre_read_cb(uint8_t rhport,
                                    uint16_t n_bytes_received,
                                    uint8_t func_id,
                                    uint8_t ep_out,
                                    uint8_t cur_alt_setting) {
    (void)rhport;
    (void)n_bytes_received;
    (void)func_id;
    (void)ep_out;
    (void)cur_alt_setting;
    return true;
}

// Called when the host sends a SET_CUR request (e.g. volume, mute).
// Return true to accept; false to stall.
bool tud_audio_set_req_ep_cb(uint8_t rhport,
                               tusb_control_request_t const *p_request,
                               uint8_t *pBuff) {
    (void)rhport;
    (void)p_request;
    (void)pBuff;
    return false;   // no endpoint control entities in V1
}

bool tud_audio_set_req_itf_cb(uint8_t rhport,
                                tusb_control_request_t const *p_request,
                                uint8_t *pBuff) {
    (void)rhport;
    (void)p_request;
    (void)pBuff;
    return false;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport,
                                   tusb_control_request_t const *p_request,
                                   uint8_t *pBuff) {
    (void)rhport;
    (void)p_request;
    (void)pBuff;
    return false;   // no feature units in V1
}

bool tud_audio_get_req_ep_cb(uint8_t rhport,
                               tusb_control_request_t const *p_request) {
    (void)rhport;
    (void)p_request;
    return false;
}

bool tud_audio_get_req_itf_cb(uint8_t rhport,
                                tusb_control_request_t const *p_request) {
    (void)rhport;
    (void)p_request;
    return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport,
                                   tusb_control_request_t const *p_request) {
    (void)rhport;
    (void)p_request;
    return false;
}
