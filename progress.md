# Progress

## 2026-09-28 — Firmware scaffold

Set up the complete build system and firmware skeleton for the RP2040-Zero.

**Build system**
- `firmware/rp2040/CMakeLists.txt` — links TinyUSB, hardware_pio, hardware_dma
- `firmware/rp2040/pico_sdk_import.cmake` — locates SDK via `PICO_SDK_PATH` env var

**USB audio (TinyUSB UAC2)**
- `src/tusb_config.h` — UAC2 playback-only, 24-bit / 48 kHz stereo
- `src/usb_descriptors.c` — device, configuration, and string descriptors; clock source + input/output terminal chain; isochronous OUT endpoint
- `src/usb_audio.h/c` — TinyUSB callbacks; polls USB RX FIFO and hands data to I2S

**I2S output**
- `src/i2s.pio` — PIO state machine: left-justified format, 32-bit frames, side-set drives BCK + WS simultaneously; BCK = 3.072 MHz at 48 kHz
- `src/i2s_audio.h/c` — repacks 3-byte USB samples into 32-bit left-justified words, pushes to PIO TX FIFO via `pio_sm_put_blocking`

**Entry point**
- `src/main.c` — `i2s_init()` → `tusb_init()` → main loop: `tud_task()` + `usb_audio_task()`

## Next

- Install toolchain (`arm-none-eabi-gcc`) and do a test build
- Confirm PCM5102A module FMT pin / jumper is set to left-justified mode
- Assign final GPIO pins for BCK / WS / DATA and update `I2S_BCK_PIN` etc. in `i2s_audio.h`
- Flash and verify USB enumeration (device should appear as audio output in OS)
- Verify I2S signal on oscilloscope before connecting DAC
- Add DMA ping-pong to `i2s_audio.c` (current blocking FIFO write is fine for initial testing)
