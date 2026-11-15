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

## 2026-11-15 — Versioning: firmware 0.1.0, hardware P1

- Rules in `docs/versioning.md`: product V1/V2, hardware P1…/A…, firmware SemVer 0.x
- Version set once in `firmware/rp2040/CMakeLists.txt`; shown in USB `bcdDevice` (0x0010),
  the serial banner and `picotool info`
- Tags `fw/v0.1.0` and `hw/P1` mark the first working USB DAC
- Headphone target decided: rated 32 Ω+, 16 Ω tolerated at reduced output; line output still open

## 2026-11-15 — Analog plan and docs cleanup

- Amplifier: stage 1 NJM4556A (breadboard, capacitor-coupled, relay mute), stage 2 OPA1622 + LM27762
  board (hw P2); both take the DAC line output at gain 1 through the same module connector
- Volume: endless encoder (no push) + firmware gain, combined with the OS volume via a UAC2 Feature
  Unit; mute (separate button/switch) deferred
- Meters: levels computed in firmware, PWM-driven; DAC always on, amp/meters on a soft-start rail
- Architecture, decisions, pinout, BOM, build log and README updated to match

## Next

Project work is organised as milestones (M2–M6 in `docs/build-log.md`), filed by subsystem; labs 0–8
in `firmware/labs/` were the learning series.


- M2: NJM4556A stage 1 on the breadboard (Part A), soft-start rail + mute relay + sequencing (Part B);
  guide in `hardware/amp-stage1/README.md`
- M3: basic 3D-printed enclosure mockup for placing boards, knob, sockets
- M4: OPA1622 + LM27762 amp PCB (hw P2), based on TIDA-01341, built for debugging and adjustment
- M5: encoder + firmware volume + Feature Unit; meters
- Optional from lab 8 stretch: PI controller, 44.1 kHz, Windows feedback format
