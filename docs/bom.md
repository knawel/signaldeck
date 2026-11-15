# Bill of Materials

This BOM is for the **prototype**, not the final production-quality device.

## A. Buy for the first digital-audio milestone

| Item | Qty | Specification / notes | Status |
|---|---:|---|---|
| Waveshare RP2040-Zero | 2 | One project board + one spare/development board | Required |
| PCM5102A I2S DAC module | 2 | Common module with L/R analog output; one spare recommended | Required |
| USB-C data cable | 1 | Must support USB data, not charging only | Required |
| Breadboard | 1 | Full or medium size | Required |
| 2.54 mm male headers | 1 strip | Breakaway | Required |
| 2.54 mm female headers | 1 strip | Useful for removable modules | Recommended |
| Jumper wires | 1 set | M-M, M-F and F-F useful | Required |
| 3.5 mm stereo socket | 1-2 | Temporary line output / testing | Recommended |

## B. Analog prototype parts

| Item | Qty | Specification / notes | Status |
|---|---:|---|---|
| Rotary encoder, no push switch | 2 | Endless, 6 mm shaft (EC11 class); buy one detented and one smooth to compare the feel | Required for the volume lab |
| 3.5 mm TRS panel socket | 1 | Stereo headphone output, **with switch contacts** (headphone detection) | Required |
| 6.35 mm TRS panel socket | 1 | Stereo headphone output, **with switch contacts** | Required |
| Perfboard | 2 | 2.54 mm pitch | Recommended |
| Hook-up wire | ~5 m | 24-26 AWG stranded, several colors | Recommended |
| Shielded audio cable | ~1 m | For analog interconnects | Recommended |
| Screw terminals | assortment | 2/3-pin | Optional |
| JST-XH or similar connectors | assortment | 2/3/4/8-pin; 8-pin for the amp module connector (`pinout.md`) | Recommended |
| Heat-shrink tubing | assortment | General assembly | Recommended |

## B2. Headphone amplifier, stage 1 (NJM4556A, breadboard)

Preliminary list (resistors not listed), from the agreed design (`architecture.md`, section 6).
Values are typical starting points; the M2 schematic in `hardware/amp-stage1/` is the final word.

| Item | Qty | Specification / notes |
|---|---:|---|
| NJM4556AD (DIP-8) | 2 | One plus a spare |
| DIP-8 socket | 2 | Swap the chip without desoldering |
| Electrolytic 1000 µF, ≥10 V, low-ESR | 4 | Output coupling (2 + 2 spare); 470 µF minimum |
| Film capacitor 1–2.2 µF | 2 | Input coupling: DAC output is centred on 0 V, amp input sits at mid-supply |
| Electrolytic 47 µF, ≥10 V | 1 | Filters the mid-supply bias point |
| Electrolytic 470 µF, ≥10 V | 1 | Bulk capacitor at the amp supply, behind the soft-start switch |
| Ferrite bead (through-hole) or 10–47 µH inductor | 1 | Keeps amp current pulses off the shared 5 V |
| 5 V signal relay, DPDT | 1 | Mute relay in series with the outputs |
| BC547 or 2N3904 | 1 | GPIO drives the relay coil |
| Dummy load resistors 33 Ω and 82 Ω, ~1 W | 2 + 2 | Test loads for the multimeter checks |

100 nF / 10 µF decoupling and 1N4148 (relay flyback) come from section C.
The soft-start switch for the audio rail (P-MOSFET or load-switch IC) is chosen in the power lab.

## C. General analog components

These are useful for amplifier, filtering, status, and later meter-driver experiments.

| Item | Suggested qty | Notes |
|---|---:|---|
| 1% metal-film resistor assortment | 1 kit | Roughly 100 Ohm-1 MOhm |
| 100 nF ceramic capacitors | 20 | Local decoupling |
| 1 uF capacitors | 10 | General analog use |
| 10 uF electrolytic capacitors | 10 | >= 16 V |
| 100 uF electrolytic capacitors | 5-10 | >= 16 V |
| 1N4148 | 20 | Meter/utility circuits |
| LEDs | 5-10 | Prototype indicators |
| 1 kOhm resistors | 10 | LEDs/general use |
| 10 kOhm trimmers | 2-5 | Meter calibration/experiments |
| LM358 DIP-8 | 2 | Prototype meter/control circuits; not necessarily final audio op-amp |
| DIP-8 sockets | 4 | Useful for experimentation |

## D. Do not buy yet

Until the electrical prototype determines requirements and physical dimensions, postpone:

- stage 2 amplifier board parts (OPA1622, LM27762 and their capacitors), until its schematic is reviewed
- DC/DC converter modules: stage 2 makes its rails with the LM27762 on the amp board
- analog VU meters
- final volume knob
- final toggle switch
- metal front panels
- enclosure
- leather adhesive
- final LEDs/bezels
- custom PCB

## E. Future microphone expansion

Not needed for V1. Possible future BOM categories:

- 3.5 mm microphone input socket
- electret/plug-in-power circuitry
- low-noise microphone preamplifier
- microphone gain potentiometer
- dedicated audio ADC

Exact components are intentionally TBD.
