# Architecture

## 1. Purpose

This document describes the current target architecture for the USB-C desktop headphone interface.
Statuses of the individual choices are in `decisions.md`; numbering rules in `versioning.md`.

The architecture is intentionally modular. During prototyping, the RP2040-Zero, the PCM5102A and
the headphone amplifier are separate modules that can be developed and replaced independently.

## 2. Top-level architecture

```text
                         USB-C (audio data + 5 V)
                                   |
                                   v
   rotary encoder  ------->  +-------------+  -------> 2x PWM --> meter drivers --> L/R meters
   (volume)                  | RP2040-Zero |  -------> AUDIO_EN, AMP_MUTE, status LEDs
   power toggle    ------->  |  USB audio  |
                             |  volume     |
                             |  meter calc |
                             +------+------+
                                    | I2S
                                    v
                             +-------------+
                             |  PCM5102A   |  line level, ~2.1 Vrms full scale
                             +------+------+
                                    |
                     +--------------+---------------+
                     |                              |
                     v                              v
          headphone amplifier (gain 1)     line output (TBD)
                     |
               +-----+-----+
               v           v
            3.5 mm      6.35 mm
```

Volume and meter levels are computed in firmware, so there is no analog volume control or analog
meter tap in the audio path.

## 3. USB and digital audio

### RP2040-Zero

Responsibilities:

- USB Audio Class 2 playback device on its own USB-C connector
- USB audio stream → I2S (PIO + DMA)
- volume: local knob and host (OS) volume, applied to the samples
- meter levels from the samples, output as PWM
- power sequencing of the amplifier (switched rail, mute)
- status LEDs

Implemented (firmware 0.1.0, hardware P1): UAC2 playback at 48 kHz / 24-bit stereo, asynchronous
with a feedback endpoint, I2S out by PIO + DMA, plus a USB serial port for status output. Works
driverless on macOS. Windows needs the feedback in 16.16 format instead of 10.14 (untested).
Volume is a fixed −24 dB (`OUTPUT_SHIFT 4`); the OS volume slider is greyed out because the
descriptors declare no volume control yet.

Do not optimize for very high sample rates until the rest of the device works.

## 4. DAC

The prototype uses a ready-made PCM5102A module, powered from the board 5V pin (module VIN).

- I2S: BCK, LRCK, DIN, common GND. No MCLK: SCK is tied to GND and the DAC's PLL derives its clock
  from BCK (see `pinout.md`).
- Output is **line level**, about 2.1 Vrms at full scale, centred on 0 V. It is not a headphone
  driver.
- XSMT (soft mute) is tied to 3V3 today; it is planned to move to a GPIO for power sequencing.
- The DAC stays powered whenever USB is connected, so the RP2040 never drives I2S lines into an
  unpowered chip.

## 5. Volume control

Requirements: a large, pleasant knob (endless, no MIN/MAX markings), local control of the level,
independent of the computer, and the OS volume slider should work too.

Design (prototype):

- **Endless rotary encoder**, without a push switch. A 6 mm shaft takes a 30–40 mm aluminium
  knob. Detented or smooth (detentless) is to be tried by feel; the exact part is TBD.
- **Firmware volume**: each sample is multiplied by a gain before it goes to the DAC.
  `total gain = host volume × knob volume`, so the two work independently.
  - knob: 1 dB steps (faster steps when turned quickly), range about 0 to −80 dB
  - host: a UAC2 **Feature Unit** with a volume control makes the macOS/Windows slider work; the
    host sends the setting, the firmware applies it
  - gain changes are ramped over a few milliseconds, so they don't click or "zipper"
  - at power-up the knob starts at a safe default level (not restored from flash): writing flash
    stalls the code running from it, and a safe start protects ears and headphones
- **Why digital volume is fine here:** USB samples are 24-bit and the DAC takes 32-bit words, so
  the multiply loses nothing audible. The hiss you hear is set by the DAC and amplifier noise
  floor, which doesn't change with the volume setting; with a gain-1 amplifier and these
  headphones it stays far below audibility.
- Consequence: anything connected to the DAC output, including a future line output, follows the
  volume.
- Option: the meters can briefly show the volume setting while the knob turns.
- **Mute is not part of the first design.** Later: a separate front-panel button or switch, and
  optionally a mute control in the Feature Unit (OS mute key).

Alternatives considered: an analog dual pot (not endless; would allow a fixed line output) and a
digitally controlled analog volume chip (more parts and wiring). Neither is planned.

## 6. Headphone amplifier

Requirements:

- stereo, gain 1: the DAC already provides enough voltage, the amplifier supplies the current
- rated for 32 Ω and above (ATH-M40x 35 Ω, DT770 Pro 80 Ω); 16 Ω tolerated at reduced output
  without damage
- input = PCM5102A line output, input impedance ≥ 10 kΩ
- runs from the switched USB 5 V rail
- no thump at power-up/down: a mute controlled by firmware

Two stages, same module interface (`decisions.md`, "Amp input / interface"):

| Stage | Hardware | Amplifier | Supply | Output | Mute |
|---|---|---|---|---|---|
| 1 | P1, breadboard | NJM4556A, gain-1 buffer | Single 5 V, ferrite + 470–1000 µF | Capacitor-coupled, about 1 Vrms max | Relay in series with the outputs |
| 2 | P2, small amp PCB | OPA1622 | LM27762: about ±4.5 V from 5 V | Ground-referenced, about 2 Vrms | OPA1622 shutdown pin + LM27762 enables |

Stage 2 follows TI reference design TIDA-01341. Before its schematic: check the OPA1622 enable-pin
logic reference (a level shifter may be needed).

## 7. Line output

A separate output for active PC speakers is under consideration (TBD). With firmware volume it
would follow the knob. Option: headphone sockets with switch contacts let the firmware detect a
plugged-in headphone and mute the line output.

## 8. Analog meters

Two physical moving-coil meters, LEFT and RIGHT.

- Levels are computed in firmware from the samples **before the knob volume** (after the host
  volume, which is already applied when the samples arrive), smoothed for VU-like movement.
- Each meter is driven by a PWM output → RC filter → simple current driver (op-amp/transistor;
  LM358 is adequate for this low-frequency job). Nothing touches the analog audio path.
- Meter processing must not block USB or DMA servicing.
- Meter model, full-scale current, illumination and driver values are TBD. Lamps may draw much more
  power than the movements.

## 9. Power

One external connection for data and power: USB-C on the RP2040-Zero.

```text
USB-C / VBUS 5 V
   |
   +----> RP2040-Zero (its own 3.3 V regulator)
   |
   +----> PCM5102A module VIN (always on)
   |
   +----> soft-start load switch  <---- AUDIO_EN (GPIO)
              |
              +----> ferrite + bulk capacitor ----> headphone amplifier
              |                                     (stage 2: LM27762 -> ±4.5 V)
              +----> meter drivers + meter lamps
```

- The RP2040 stays powered and enumerated while USB is connected.
- The front-panel power toggle is read by the firmware, which runs the sequence: rail on → wait
  until ready → unmute; mute → rail off. The same happens on USB suspend/disconnect.
- The soft-start switch limits inrush into the bulk capacitors (USB allows only about 10 µF directly
  on VBUS). The bulk capacitor steadies the rail; it doesn't supply the average power.
- The USB descriptor declares 100 mA today. Raise it (up to 500 mA) once the power budget is
  measured; meter lamps are the largest unknown.

## 10. Front panel concept

Two metal instrument-style subpanels (details in `enclosure.md`).

### Instrument panel

- LEFT analog meter
- RIGHT analog meter
- USB status indicator
- AUDIO power indicator

### Control panel

- large volume knob (endless encoder)
- 3.5 mm headphone socket
- 6.35 mm headphone socket

A separate physical toggle switch may be positioned outside/below the two subpanels.

## 11. Microphone expansion

Not required for V1, but the design should not make it unnecessarily difficult to add later.

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
