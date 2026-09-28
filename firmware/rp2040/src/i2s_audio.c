#include "i2s_audio.h"
#include "i2s.pio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <string.h>
#include <stdio.h>

static PIO  s_pio = pio0;
static uint s_sm  = 0;

void i2s_init(void) {
    uint offset = pio_add_program(s_pio, &i2s_out_program);

    i2s_out_program_init(s_pio, s_sm, offset,
                         I2S_DATA_PIN,
                         I2S_BCK_PIN,
                         (float)I2S_SAMPLE_RATE);

    printf("i2s_init: BCK=GPIO%d WS=GPIO%d DATA=GPIO%d @ %d Hz\n",
           I2S_BCK_PIN, I2S_WS_PIN, I2S_DATA_PIN, I2S_SAMPLE_RATE);
}

// USB audio delivers 24-bit stereo as interleaved 3-byte little-endian samples:
//   [L2 L1 L0] [R2 R1 R0] [L2 L1 L0] ...
//
// The PIO program expects left-justified 32-bit words (sample in bits [31:8]):
//   word = (sample_24bit << 8)
//
// This function repacks and pushes each word into the PIO TX FIFO.
// pio_sm_put_blocking() stalls if the FIFO is full, which provides natural
// flow control — it will block for at most a few microseconds at 48 kHz.
//
// TODO: replace with DMA ping-pong for glitch-free playback.
void i2s_write(const uint8_t *buf, size_t nbytes) {
    size_t nsamples = nbytes / I2S_BYTES_PER_SAMPLE;

    for (size_t i = 0; i < nsamples; i++) {
        const uint8_t *p = buf + i * I2S_BYTES_PER_SAMPLE;

        // Reconstruct 24-bit sample from 3 little-endian bytes
        uint32_t word = ((uint32_t)p[2] << 24)
                      | ((uint32_t)p[1] << 16)
                      | ((uint32_t)p[0] <<  8);

        pio_sm_put_blocking(s_pio, s_sm, word);
    }
}

void i2s_mute(void) {
    // Drain the TX FIFO by pushing silence words.
    // The PIO state machine keeps running; it just outputs zeros.
    for (int i = 0; i < 8; i++) {
        pio_sm_put(s_pio, s_sm, 0);  // non-blocking; drops if full
    }
}
