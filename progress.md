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

## 2026-11-15 — Lab 8 promoted to the main firmware

`firmware/rp2040` now holds lab 8 Part C, replacing the September scaffold (which was never built).
Plays music from a Mac as a UAC2 sound card: 48 kHz / 24-bit, I2S (not left-justified) to the
PCM5102A on GP0–2, DMA ping-pong, asynchronous feedback with a P controller on the FIFO level.

- One `main.c` plus `usb_descriptors.c/.h`, `tusb_config.h`, `i2s.pio`; own CMake and VS Code setup
- Workaround for a TinyUSB 0.18 RP2040 panic on stream restart (`tud_audio_set_itf_close_EP_cb`)
- Flash with `picotool load -x build/signaldeck.elf -f --vid 0xcafe --pid 0x4008` (the Run task does it)
- Measured: playback drifts ~300 ppm vs the Mac without the controller; with KP 8 the FIFO settles
  ~97 frames below target, 0 short / 0 overflow

## Next

- Analog volume control (now: −24 dB digital, `OUTPUT_SHIFT`)
- Optional from lab 8 stretch: PI controller, 44.1 kHz, Windows feedback format
