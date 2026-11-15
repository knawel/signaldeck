# Build Log

Use this file as a concise engineering notebook. Record what was actually built and measured, including failures.

---

## Milestone 0 - Project definition

**Status:** Done

### Current architecture

- RP2040-Zero USB controller
- PCM5102A DAC module
- analog stereo volume
- separate headphone amplifier (TBD)
- 3.5 mm + 6.35 mm outputs
- two analog L/R meters
- single USB-C cable for power and audio

### Next objective

Establish the smallest possible working digital audio chain:

```text
Computer -> USB-C -> RP2040-Zero -> I2S -> PCM5102A -> line output
```

### Success criteria

- host recognizes RP2040 as an audio playback device
- stable stereo playback
- correct left/right channels
- no obvious dropouts or digital artifacts
- PCM5102A produces clean line-level output

---

## Milestone 1 - USB audio to PCM5102A

**Status:** Working (playback); some tests still open  
**Date:** 2026-11-15  
**Hardware revision:** P1 (breadboard)  
**Firmware:** 0.1.0, tag `fw/v0.1.0` (`firmware/rp2040`, promoted from lab 8 Part C in 65770cd)

### Wiring

GP0 → BCK, GP1 → LCK, GP2 → DIN, 5V → VIN on its own jumper, GND; SCK → GND (no MCLK);
FLT, DEMP, FMT → GND, XSMT → 3V3. Details in [`pinout.md`](pinout.md).

### Firmware configuration

- UAC2, 48 kHz, 24-bit stereo (3-byte subslots), max packet 294 bytes (49 frames)
- asynchronous isochronous OUT endpoint + feedback endpoint (10.14 on the wire, for macOS)
- TinyUSB FIFO 16 packets (784 frames), playback starts at half full (392)
- feedback = own measured rate (48.00307 samples/frame) + P controller on the averaged FIFO level, KP 8
- I2S by PIO at 6.144 MHz, DMA ping-pong of 2 × 2 ms buffers
- output attenuated −24 dB in software (`OUTPUT_SHIFT 4`)
- Pico SDK 2.3.1 / TinyUSB 0.18; status over USB serial

### Host

- OS: macOS (version TBD)
- sample format: 24-bit / 48 kHz; Audio MIDI Setup shows "2 ch 24-bit Integer 48.0 kHz"

### Tests

- [x] USB enumeration ("signaldeck USB DAC")
- [x] continuous playback (YouTube, minutes, 0 short / 0 overflow with the controller)
- [ ] left channel correct
- [ ] right channel correct
- [ ] silence when no stream is active
- [ ] DAC output measured
- [x] listening test through powered speakers
- [x] pause / resume and switching outputs on the host (after the TinyUSB workaround)

### Measurements

Rough readings from the status line:

- fixed feedback 48.00000: the FIFO drains ~14 frames/s, so the board plays ~300 ppm fast against the Mac's clock
  (PIO divider alone accounts for +64 ppm; the rest is crystal mismatch)
- fixed feedback 48.00307 (own computed rate): still drains, more slowly
- P controller, KP 8: FIFO settles ~295 frames (~97 below target), feedback ≈ 48.0145
- KP 2000: settles near target but the feedback jitters strongly; KP 8 kept

### Problems / observations

- TinyUSB 0.18 on the RP2040 panicked ("ep 01 was already available") whenever the host
  restarted the stream after a pause. Worked around in `tud_audio_set_itf_close_EP_cb()`.
- picotool's `-f` ignores devices with a non-Raspberry-Pi vendor ID: flash with
  `--vid 0xcafe --pid 0x4008`.
- A refill after running dry is only ~8–10 ms of silence: hard to hear in music, obvious on a sine tone.
- Starving the main loop (`sleep_ms(5)`) loses USB packets and makes the DMA replay stale buffers.

### Changes for next revision

- firmware volume (encoder + UAC2 Feature Unit), then retire the fixed −24 dB attenuation
- optional: PI controller (removes the steady FIFO offset), 44.1 kHz, Windows feedback format

---

## Milestone 2 - Headphone amplifier stage 1 (NJM4556A) + power sequencing

**Status:** Not started (lab 9: amplifier; lab 10: switched rail, mute, sequencing)

Planned tests (multimeter, dummy loads 33 Ω / 82 Ω, then headphones):

- [ ] idle supply current near the datasheet value, chip cool (no oscillation)
- [ ] DC offset across the load
- [ ] output level vs digital level at 1 kHz (or 400 Hz): where clipping starts
- [ ] no hum/hiss audible at maximum with nothing playing
- [ ] no thump at power-up/down and USB plug/unplug
- [ ] 3.5 mm and 6.35 mm outputs
- [ ] 16 Ω accidental load: reduced output, no damage
- [ ] measured power budget → USB `bMaxPower`

---

## Milestone 3 - Volume + analog meters

**Status:** Not started

Planned tests:

- [ ] encoder: 1 dB steps, no missed or reversed steps
- [ ] OS volume slider works (UAC2 Feature Unit), independent of the knob
- [ ] no clicks when the volume changes
- [ ] meter levels independent of the knob, LEFT/RIGHT independence
- [ ] useful attack and decay behavior
- [ ] calibration repeatability
- [ ] meter illumination power/noise

---

## Milestone 4 - Mechanical prototype

**Status:** Not started

Planned work:

- verify real component clearances
- define enclosure dimensions
- position two front subpanels
- confirm meter size
- design metal panels
- test red leather treatment/accent
