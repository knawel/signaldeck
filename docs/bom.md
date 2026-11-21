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
| Rotary encoder, no push switch | 2 | **Bourns PEC11R-4220F-N0024**: 24 detents / 24 pulses (one full quadrature cycle per click), M7×0.75 bushing 7 mm, 6 mm D shaft 20 mm, nut + washer. One plus a spare; smooth variant dropped | M5; **ordered 2026-11-21** |
| 3.5 mm TRS panel socket | 1 | **Amphenol ACJS-MV35-3S**: stereo, switched, metal M-series thread + nut, 4 terminals (vertical PCB pins, wired). Also the breadboard socket during M2 | M3; **ordered 2026-11-21** |
| 6.35 mm TRS panel socket | 1 | **Neutrik NMJ6HC-S**: stereo, 3 switching contacts, solder lugs, rear mount, panel < 4.7 mm, nut included. Datasheet "chassis shape 11.4 mm": measure the hole | M3; **ordered 2026-11-21** |
| Perfboard | 5-6 | 2.54 mm pitch, double-sided FR4 with plated holes, 5 × 7 cm (amp ~5 × 7 cm); buy locally, far cheaper than Mouser | Recommended |
| Hook-up wire | ~5 m | 24-26 AWG stranded, several colors | Recommended |
| Shielded audio cable | ~1 m | For analog interconnects | Recommended |
| Screw terminals | assortment | 2/3-pin | Optional |
| JST-XH or similar connectors | assortment | 2/3/4/8-pin; 8-pin for the amp module connector (`pinout.md`) | Recommended |
| Heat-shrink tubing | assortment | General assembly | Recommended |
| Breadboard (second) | 1 | Full or medium size; the M1 board holds the RP2040 and DAC, this one the amp | M2; **in stock** |
| Toggle switch (prototype) | 1 | **E-Switch 100DP1T1B1M1REH**: miniature DPDT ON-ON, gold contacts (logic level), solder lugs, 1/4"-40 UNS bushing → 6.4 mm hole; read by firmware. The final panel toggle is still in D | M2 Part B / M3; **ordered 2026-11-21** |

## B2. Headphone amplifier, stage 1 (NJM4556A, breadboard)

Final values for Part A, from the answers in `hardware/amp-stage1/README.md` section 3.2 (Q1-Q6).
Quantities cover both channels; spares are noted. Resistors are ordinary 1/4 W metal film
unless noted. **In stock:** 1/4 W resistor kit, including 4.7 Ω, 560 Ω and 820 Ω.

| Ref | Item | Qty | Specification / notes |
|---|---|---:|---|
| U1 | NJM4556AM (DMP8, SMD, 1.27 mm pitch) | 3 | One plus spares. The DIP-8 NJM4556AD is NRND at Nisshinbo and out of stock; same die and pinout. **Ordered** |
| | Chip Quik IPC0142, SOP-8 to DIP-8 adapter | 4 | U1 is soldered on, then plugs in as a DIP-8; pin numbers unchanged. One per op-amp type (NJM4556AM, OPA1688, OPA1656, see B3) + a spare. Header pins may not be included: use 2.54 mm headers. **Ordered** |
| | DIP-8 socket | 2 | Holds the adapter on the perfboard build. **In stock** |
| R_b | Resistor 100 kΩ | 2 | Input bias to VREF; the DAC's load (Q2). Kit, or the 0.1 % YAGEO from B3 |
| R_top, R_bot | Resistor 10 kΩ | 2 | VREF divider (Q3) |
| R_s | Resistor 4.7 Ω | 2 | Output series resistor |
| R_bleed | Resistor 560 Ω | 2 | Charges C_out within ~3 s (Q4) |
| R_base | Resistor 820 Ω | 1 | Relay driver base, ~3.2 mA from a 3.3 V GPIO (Q5) |
| C_in | KEMET R82DC4100Z360K, film 1 µF 63 V, LS 5 mm | 4 | Input coupling, corner 1.6 Hz (Q2); film, not ceramic. **Ordered** |
| C_out | Panasonic EEU-FR1E102L, 1000 µF 25 V low-ESR, 10 × 25 mm, LS 5 mm | 10 | Output coupling, corner 4.3 Hz at 32 Ω (Q1). **Ordered** |
| C_ref | Vishay MAL214270479E3, 47 µF 35 V | 10 | Filters VREF (Q3). **Ordered** |
| C_bulk | Panasonic EEU-FM1H471, 470 µF 50 V | 10 | Bulk capacitor at the amp supply. **Ordered** |
| | 100 nF ceramic + 10 µF electrolytic | 1 + 1 | Decoupling at U1 pins 8/4. **In stock** |
| FB1 | Fastron 06H-102X-00, 6-hole ferrite bead | 1 | Keeps HF noise off the amp rail. Chosen over a 10-47 µH inductor: L + 470 µF would resonate at ~1-2 kHz, right where USB's 1 kHz frame noise sits. **Ordered** |
| K1 | Panasonic TQ2-5V (5 V DPDT signal relay) | 3 | Mute relay, coil 178 Ω / 28 mA (Q5); gold-clad contacts. One plus spares. **Ordered** |
| Q1 | BC547 (or 2N3904) | 1 | GPIO drives the relay coil. **In stock** (transistor kit) |
| D1 | 1N4148 | 1 | Flyback diode across the coil. **In stock** |
| | Dummy loads: Vishay PR01 33 Ω 1 % 1 W, KOA SPR1 82 Ω 5 % 1 W | 10 + 10 | Test loads (steps A3-A7); not in the signal path, so type and 5 % do not matter. **Ordered** |
| | Same Sky SP-3533-02, 3.5 mm stereo plug | 1 | **Plug-in dummy load**: 33 Ω tip→sleeve and ring→sleeve soldered on; tests amp → C_out → relay → jack. **Ordered** |
| | 3.5 mm socket on the breadboard | 1 | The ACJS-MV35-3S from B with three short wires, until it moves to the panel |
| | Cheap headphones | 1 | First listening test (A6), not the DT770 / M40x. **Still to get** (not Mouser) |
| | Flux (pen or no-clean gel) | 1 | For soldering U1 on the adapter. **Get locally if missing** |

**M2 order placed 2026-11-21 at Mouser** (€75.81, free delivery from €75): everything marked
**Ordered** here and in sections B and B3.
The soft-start switch for the audio rail (P-MOSFET or load-switch IC) is chosen in Part B; not yet.

## B3. Experiments (ordered with M2)

Extras for learning; not needed for M2.

| Item | Qty | Use |
|---|---:|---|
| TI OPA1688ID (SOIC-8) | 2 | Swap-in op-amp for stage 1: rail-to-rail output, 4.5 V minimum supply, input best below V+ − 2 V (lower VREF to ~1.8-2 V) |
| TI OPA1656ID (SOIC-8) | 1 | Swap-in: ultra-low noise, 100 mA output; same pinout |
| YAGEO MFP-25BTD52-100K, 100 kΩ 0.1 % | 10 | Matched L/R parts, reference values |
| Cree / Kingbright 5 mm LEDs (amber 590 nm, red, blue) | 13 | Status / meter-lamp colour trials; amber looks like old incandescent VU lamps |
| Microchip 24LC256-I/P, I2C EEPROM 32 KB, DIP-8 | 1 | I2C practice with the small I2C display; addresses 0x50-0x57 |
| TI SN74HC595N, 8-bit shift register, DIP-16 | 1 | PIO output exercise: 8 outputs from 3 GPIOs |

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
