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

Final values for Part A, from the answers in `hardware/amp-stage1/README.md` section 3.2 (Q1-Q6).
Quantities cover both channels; spares are noted. Resistors are ordinary 1/4 W metal film
unless noted; check the assortment (section C) for 4.7 Ω, 560 Ω and 820 Ω, which kits often skip.

| Ref | Item | Qty | Specification / notes |
|---|---|---:|---|
| U1 | NJM4556AD (DIP-8) | 2 | One plus a spare |
| | DIP-8 socket | 2 | Swap the chip without desoldering |
| R_b | Resistor 100 kΩ | 2 | Input bias to VREF; the DAC's load (Q2) |
| R_top, R_bot | Resistor 10 kΩ | 2 | VREF divider (Q3) |
| R_s | Resistor 4.7 Ω | 2 | Output series resistor |
| R_bleed | Resistor 560 Ω | 2 | Charges C_out within ~3 s (Q4) |
| R_base | Resistor 820 Ω | 1 | Relay driver base, ~3.2 mA from a 3.3 V GPIO (Q5) |
| C_in | Film capacitor 1 µF, 5 mm pitch | 2 (+1) | Input coupling, corner 1.6 Hz (Q2); film, not ceramic |
| C_out | Electrolytic 1000 µF, ≥10 V, low-ESR | 2 (+2) | Output coupling, corner 4.3 Hz at 32 Ω (Q1) |
| C_ref | Electrolytic 47 µF, ≥10 V | 1 | Filters VREF (Q3) |
| C_bulk | Electrolytic 470 µF, ≥10 V | 1 | Bulk capacitor at the amp supply |
| | 100 nF ceramic + 10 µF electrolytic | 1 + 1 | Decoupling at U1 pins 8/4 (section C stock) |
| FB1 | Ferrite bead (through-hole) or 10-47 µH inductor | 1 | Keeps amp current pulses off the shared 5 V |
| K1 | Panasonic TQ2-5V (5 V DPDT signal relay) | 1 | Mute relay, coil 178 Ω / 28 mA (Q5); gold-clad contacts |
| Q1 | BC547 (or 2N3904) | 1 | GPIO drives the relay coil. **In stock** (transistor kit) |
| D1 | 1N4148 | 1 | Flyback diode across the coil. **In stock** |
| | Dummy load resistors 33 Ω and 82 Ω, ~1 W | 2 + 2 | Test loads (steps A3-A7) |
| | 3.5 mm stereo socket, breadboard breakout | 1 | Headphone connection on the breadboard |
| | Cheap headphones | 1 | First listening test (A6), not the DT770 / M40x |

Buy the NJM4556AD and TQ2-5V from a franchised distributor (Mouser, DigiKey, TME, Farnell).
The soft-start switch for the audio rail (P-MOSFET or load-switch IC) is chosen in Part B; not yet.

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
