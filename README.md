# USB-C Desktop Headphone Interface

A compact, repairable desktop USB audio interface built around an **RP2040-Zero** and **PCM5102A** DAC.

The device is intended to combine a useful everyday headphone output with a hands-on electronics/firmware project. The design deliberately favors modular construction, understandable circuitry, physical controls, and an instrument-like front panel over extreme miniaturization.

## Target features

### V1

- Single USB-C connection for **audio data and power**
- RP2040-Zero as the USB Audio device/controller
- PCM5102A I2S DAC
- Stereo headphone amplifier (rated for 32 Ω and above)
- Large endless volume knob (rotary encoder); the OS volume slider works too
- 3.5 mm stereo headphone output
- 6.35 mm stereo headphone output
- Physical audio-power switch
- USB / audio-power status indicators
- Two analog L/R level meters, driven by the firmware
- Metal front panel with two aircraft/instrument-style subpanels
- Optional red leather exterior/accent panels

### Possible V2

- 3.5 mm microphone input
- Microphone gain control
- Dedicated audio ADC
- Simultaneous USB playback and recording
- Custom PCB replacing prototype modules
- Line output for active speakers (undecided)
- Mute button or switch
- Additional firmware-controlled indicators

## Initial audio target

The first firmware milestone is deliberately conservative:

- USB Audio Class device
- Stereo playback
- 24-bit / 48 kHz target
- I2S output to PCM5102A

Higher sample rates can be considered after the basic implementation is stable.

**Status (2026-11-15):** this milestone works on the breadboard prototype: the Mac plays through
`firmware/rp2040` into the PCM5102A. See [`docs/build-log.md`](docs/build-log.md).

## System overview

```text
USB-C ---> RP2040-Zero ---I2S---> PCM5102A DAC ---> headphone amplifier (gain 1) ---> 3.5 mm + 6.35 mm
             |    ^                                 (stage 1 NJM4556A, stage 2 OPA1622)
             |    +--- rotary encoder (volume), power toggle
             +-------> PWM ---> L/R meter drivers ---> analog meters
```

Volume and meter levels are computed in the firmware. See [`docs/architecture.md`](docs/architecture.md) for details.

## Versions

Firmware **0.1.0** on hardware **P1** (breadboard). Rules in [`docs/versioning.md`](docs/versioning.md).

## Repository structure

```text
.
├── README.md
├── progress.md          dated progress notes
├── docs/
│   ├── architecture.md  target architecture
│   ├── decisions.md     decision log with status
│   ├── versioning.md    product / hardware / firmware numbering
│   ├── pinout.md        wiring of the current hardware revision
│   ├── bom.md           parts
│   ├── build-log.md     what was built and measured
│   ├── analog-analysis.md  amplifier/power background
│   └── enclosure.md     enclosure and front-panel concept
├── firmware/
│   ├── labs/            learning labs 0–8 with labbooks
│   └── rp2040/          the device firmware
├── hardware/
│   ├── datasheets/
│   ├── schematics/
│   └── kicad/
├── mechanical/
│   ├── drawings/
│   ├── enclosure/
│   └── front-panel/
└── images/
```

## Development philosophy

1. Make USB audio -> I2S -> DAC work first.
2. Verify clean line-level analog output.
3. Add the headphone amplifier (NJM4556A on the breadboard, later an OPA1622 board) and power sequencing.
4. Add encoder volume, analog level meters and indicators.
5. Prototype the complete device before committing to enclosure dimensions.
6. Only then design a custom PCB and final mechanical assembly.

The project should remain modular enough that the USB controller, DAC, amplifier, and meter-driver sections can be tested or replaced independently.
