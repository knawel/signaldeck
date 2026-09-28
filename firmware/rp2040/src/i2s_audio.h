#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <stdint.h>
#include <stddef.h>

// I2S GPIO assignments — TBD; must be consecutive BCK/WS (PIO constraint)
// Verify against actual breadboard wiring before first flash.
#define I2S_BCK_PIN   10   // bit clock (BCLK)
#define I2S_WS_PIN    11   // word select / LRCLK  (must be BCK + 1)
#define I2S_DATA_PIN  12   // serial data out (DIN on PCM5102A)

#define I2S_SAMPLE_RATE   48000
#define I2S_BIT_DEPTH     24
#define I2S_CHANNELS      2

// Initialise PIO state machine and DMA for I2S output.
void i2s_init(void);

// Write interleaved L/R 24-bit samples (packed in 3-byte little-endian words)
// from the USB audio buffer to the I2S FIFO. Non-blocking: drops samples
// if the I2S FIFO is full (indicates underrun condition).
void i2s_write(const uint8_t *buf, size_t nbytes);

// Zero the I2S output (send silence) — called when the USB stream stops.
void i2s_mute(void);

#endif // I2S_AUDIO_H
