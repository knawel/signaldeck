# USB-C Desktop Headphone Interface

A compact, repairable desktop USB audio interface built around an **RP2040-Zero** and **PCM5102A** DAC.

The device is intended to combine a useful everyday headphone output with a hands-on electronics/firmware project. The design deliberately favors modular construction, understandable circuitry, physical controls, and an instrument-like front panel over extreme miniaturization.

## Target features

### V1

- Single USB-C connection for **audio data and power**
- RP2040-Zero as the USB Audio device/controller
- PCM5102A I2S DAC
- Stereo headphone amplifier
- Analog stereo volume control
- 3.5 mm stereo headphone output
- 6.35 mm stereo headphone output
- Physical audio-power switch
- USB / audio-power status indicators
- Two analog L/R level meters
- Metal front panel with two aircraft/instrument-style subpanels
- Optional red leather exterior/accent panels

### Possible V2

- 3.5 mm microphone input
- Microphone gain control
- Dedicated audio ADC
- Simultaneous USB playback and recording
- Custom PCB replacing prototype modules
- Additional firmware-controlled indicators

## Initial audio target

The first firmware milestone is deliberately conservative:

- USB Audio Class device
- Stereo playback
- 24-bit / 48 kHz target
- I2S output to PCM5102A

Higher sample rates can be considered after the basic implementation is stable.

## System overview

```text
USB-C
  |
  v
RP2040-Zero
  |
  | I2S
  v
PCM5102A DAC
  |
  +------------------> L/R meter drivers --> analog meters
  |
  v
10k dual-gang logarithmic volume control
  |
  v
Headphone amplifier
  |
  +--> 3.5 mm TRS
  +--> 6.35 mm TRS
```

See [`docs/architecture.md`](docs/architecture.md) for details.

## Repository structure

```text
.
├── README.md
├── docs/
│   ├── architecture.md
│   ├── bom.md
│   ├── decisions.md
│   ├── pinout.md
│   └── build-log.md
├── firmware/
│   └── rp2040/
├── hardware/
│   ├── schematics/
│   └── pcb/
└── mechanical/
    ├── enclosure/
    └── front-panel/
```

## Development philosophy

1. Make USB audio -> I2S -> DAC work first.
2. Verify clean line-level analog output.
3. Add volume control and headphone amplifier.
4. Add analog level meters and indicators.
5. Prototype the complete device before committing to enclosure dimensions.
6. Only then design a custom PCB and final mechanical assembly.

The project should remain modular enough that the USB controller, DAC, amplifier, and meter-driver sections can be tested or replaced independently.
