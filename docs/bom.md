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
| Dual-gang potentiometer | 1-2 | A10K / 10 kOhm logarithmic stereo | Required later |
| 3.5 mm TRS panel socket | 1 | Stereo headphone output | Required later |
| 6.35 mm TRS panel socket | 1 | Stereo headphone output | Required later |
| Headphone amplifier | 1 | **TBD: do not order final part blindly** | TBD |
| Perfboard | 2 | 2.54 mm pitch | Recommended |
| Hook-up wire | ~5 m | 24-26 AWG stranded, several colors | Recommended |
| Shielded audio cable | ~1 m | For analog interconnects | Recommended |
| Screw terminals | assortment | 2/3-pin | Optional |
| JST-XH or similar connectors | assortment | 2/3/4-pin | Recommended |
| Heat-shrink tubing | assortment | General assembly | Recommended |

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

- final headphone amplifier IC/module
- DC/DC converter or split-rail supply
- expensive audio potentiometer
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
