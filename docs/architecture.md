# Architecture

## 1. Purpose

This document describes the current target architecture for the USB-C desktop headphone interface.

The architecture is intentionally modular. During prototyping, the RP2040-Zero and PCM5102A are separate modules and the analog electronics can be developed independently.

## 2. Top-level architecture

```text
                            USB-C
                       AUDIO DATA + 5 V
                              |
                              v
                      +---------------+
                      |  RP2040-Zero  |
                      | USB Audio MCU |
                      +-------+-------+
                              |
                              | I2S
                              v
                      +---------------+
                      |   PCM5102A    |
                      |      DAC      |
                      +-------+-------+
                              |
                         analog L/R
                              |
                +-------------+-------------+
                |                           |
                v                           v
        L/R meter drivers          stereo volume control
                |                           |
                v                           v
        analog L/R meters           headphone amplifier
                                            |
                                      +-----+-----+
                                      |           |
                                      v           v
                                   3.5 mm      6.35 mm
```

## 3. USB and digital audio

### RP2040-Zero

Responsibilities:

- USB device connection through its USB-C connector
- USB Audio Class playback
- conversion of USB audio stream into I2S
- optional control of status LEDs
- optional mute/control signals for the analog subsystem

Initial firmware target:

- stereo output
- 24-bit / 48 kHz
- driverless operation where supported by the host OS

Do not optimize for very high sample rates until the basic audio path is stable.

## 4. DAC

### PCM5102A module

The prototype uses a ready-made PCM5102A module.

Expected digital signals:

- BCLK
- LRCLK / WS
- DATA
- common GND

The selected module/configuration should not require an external MCLK from the RP2040.

The DAC output is treated as **line level**. It must not be assumed to be a headphone driver.

## 5. Volume control

Initial target:

- 10 kOhm dual-gang logarithmic potentiometer
- analog volume control
- placed between the DAC output and headphone amplifier

The final potentiometer model is TBD. A low-cost part is acceptable during prototyping; channel tracking should be evaluated before selecting the final component.

## 6. Headphone amplifier

The exact V1 amplifier topology is **TBD**.

Requirements:

- stereo
- compatible with the available USB-derived power architecture
- stable with common headphone loads
- suitable for approximately 16-80 Ohm headphones as the initial target
- low noise at normal listening levels
- enough current for low-impedance headphones
- output protection / sensible startup and shutdown behavior

Do not assume that NJM4556, TPA6120A2, or any previously discussed amplifier is the final choice until a schematic and supply topology have been reviewed.

## 7. Analog meters

Two physical moving-coil meters are planned:

- LEFT
- RIGHT

Preferred signal tap: DAC line output **before the volume control**.

Reason: meter movement should represent source/audio level and remain independent of headphone listening volume.

Conceptual path:

```text
DAC L ----+----> volume L
          |
          +----> high-impedance meter driver L ----> LEFT meter

DAC R ----+----> volume R
          |
          +----> high-impedance meter driver R ----> RIGHT meter
```

The meter driver should not significantly load the DAC output.

Exact rectifier, attack/decay constants, calibration level, meter sensitivity, and op-amp are TBD.

## 8. Power

The device has one external connection for both data and power: USB-C on the RP2040-Zero.

Conceptual distribution:

```text
USB-C / VBUS 5 V
       |
       +----> RP2040-Zero / digital section
       |
       +----> switched audio power rail
                  |
                  +----> DAC power
                  +----> headphone amplifier supply circuitry
                  +----> meter drivers
                  +----> meter illumination
                  +----> AUDIO indicator
```

Preferred behavior:

- RP2040 remains powered while USB is connected.
- Front-panel POWER/AUDIO switch controls the analog/audio subsystem.
- Host USB enumeration therefore does not necessarily disappear when the audio power switch is off.

This behavior must be validated electrically. In particular, avoid unintentionally driving an unpowered DAC through I/O pins.

## 9. Front panel concept

Two metal instrument-style subpanels are planned.

### Instrument panel

- LEFT analog meter
- RIGHT analog meter
- USB status indicator
- AUDIO power indicator

### Control panel

- large volume knob
- 3.5 mm headphone socket
- 6.35 mm headphone socket

A separate physical toggle switch may be positioned outside/below the two subpanels.

Red leather may be used around the metal panels or on the enclosure shell rather than directly underneath controls.

## 10. Microphone expansion

Microphone input is explicitly **not required for the first playback prototype**, but the design should avoid making it unnecessarily difficult to add later.

Possible V2 path:

```text
3.5 mm microphone
       |
       v
plug-in power / microphone front end
       |
       v
low-noise preamplifier
       |
       v
dedicated audio ADC
       |
       | I2S / suitable digital interface
       v
RP2040
       |
       v
USB audio capture endpoint
```

Do not use the RP2040 internal ADC as the assumed final audio ADC.
