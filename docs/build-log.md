# Build Log

Use this file as a concise engineering notebook. Record what was actually built and measured, including failures.

---

## Milestone 0 - Project definition

**Status:** In progress

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

**Date:** TBD  
**Hardware revision:** Prototype / breadboard  
**Firmware commit:** TBD

### Wiring

TBD

### Firmware configuration

TBD

### Host

- OS: TBD
- sample format: 24-bit / 48 kHz target

### Tests

- [ ] USB enumeration
- [ ] continuous playback
- [ ] left channel correct
- [ ] right channel correct
- [ ] silence when no stream is active
- [ ] DAC output measured
- [ ] listening test through known line-level amplifier/powered speakers

### Measurements

TBD

### Problems / observations

TBD

### Changes for next revision

TBD

---

## Milestone 2 - Volume + headphone amplifier

**Status:** Not started

Planned tests:

- [ ] volume range
- [ ] channel tracking
- [ ] noise/hum at minimum and normal volume
- [ ] startup/shutdown transient behavior
- [ ] 3.5 mm output
- [ ] 6.35 mm output
- [ ] representative low-impedance headphone load

---

## Milestone 3 - Analog meters

**Status:** Not started

Planned tests:

- [ ] high-impedance tap does not affect audio
- [ ] LEFT/RIGHT independence
- [ ] useful attack behavior
- [ ] useful decay behavior
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
