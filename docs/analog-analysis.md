# Analog section — background and analysis

Updated: 2026-11-15. The decisions themselves live in `decisions.md` and `architecture.md`; this file
keeps the reasoning and numbers behind them.

## Working baseline (hardware P1, firmware 0.1.0)

- RP2040-Zero -> I2S -> PCM5102A module; macOS identifies “signaldeck USB DAC”.
- 24-bit/48kHz stereo, PIO/DMA, asynchronous USB feedback.
- GP0 = BCK, GP1 = LRCK, GP2 = DIN. SCK/FLT/DEMP/FMT grounded; XSMT at 3V3.
- The module's VIN is powered from the board 5V pin; this is not the bare DAC's supply pin.
- Fixed attenuation `OUTPUT_SHIFT=4` (about −24 dB). No volume control, OS slider greyed out.
- USB configuration declares 100 mA.

## Requirements (decided 2026-11-15)

- Headphones: ATH-M40x (35 Ω), DT770 Pro (80 Ω), generic PC headphones. Rated for 32 Ω and above;
  accidental 16 Ω tolerated at reduced output, no damage.
- USB power only; complete the electronics before the enclosure/custom PCB.
- No oscilloscope: verification by multimeter, dummy loads and listening (see below).

## How much output is needed

The PCM5102A gives about 2.1 Vrms at full scale. Approximate sensitivities (not datasheet-checked),
for 110 dB SPL peaks:

| Headphone | Power | Voltage | Current |
|---|---:|---:|---:|
| ATH-M40x, 35 Ω, ~98 dB/mW | ~16 mW | ~0.75 Vrms | ~21 mA |
| DT770 Pro 80 Ω, ~96 dB/mW | ~25 mW | ~1.4 Vrms | ~18 mA |

So the DAC already has enough voltage; the amplifier is a **gain-1 current buffer**. The fixed −24 dB
today (about 0.13 Vrms) is already loud enough for normal listening.

Current demand by load (Ohm's law, not amplifier performance):

| Output | 32-ohm load | 16-ohm load |
|---|---:|---:|
| 1Vrms | 31mA RMS / 44mA peak | 63mA RMS / 88mA peak |
| 2Vrms | 63mA RMS / 88mA peak | 125mA RMS / 177mA peak |

## Amplifier: why two stages

| | Stage 1: NJM4556A, single 5 V | Stage 2: OPA1622 + LM27762 |
|---|---|---|
| Supply | VBUS through ferrite + bulk capacitor | LM27762: ±1.5 to ±5 V adjustable (≈ ±4.5 V on USB), 250 mA per rail, 2 MHz charge pump + LDOs |
| Output | Capacitor-coupled, about 1 Vrms max | Ground-referenced, about 2 Vrms (swing vs load still to check) |
| Drive | 70 mA advertised | +145 / −130 mA; −119 dB THD+N at 100 mW into 32 Ω |
| Pop control | Mute relay | OPA1622 shutdown pin (designed to avoid clicks), LM27762 enables + power-good |
| Packages | DIP-8, hand-built | VSON-10 3×3 mm, WSON-12 3×2 mm: PCB with assembly or hot plate |
| Cost / time | ~€5–10, a weekend | ~€40–80 including a board order, ~4–6 weeks; each respin ~€30–60 and 2–3 weeks |

Stage 1 is the learning build and gives working hardware for the sequencing and volume firmware;
stage 2 (TI TIDA-01341 as the layout reference) is the target amplifier. MAX97220A and ready-made
modules were considered and dropped. Open check for stage 2: the OPA1622 enable-pin logic reference.

16 Ω on stage 2: about 350 mA peak per rail at 2 Vrms on both channels exceeds the LM27762's
250 mA, so its current limit clips — acceptable as "reduced output", or capped in firmware.

## Supply bulk capacitance

A large capacitor steadies the rail and keeps amp current pulses off the 5 V shared with the RP2040
and DAC; it does not supply the average power (USB has plenty). Holding a 100 mA, 12.5 ms bass
half-cycle within 0.1 V would take ~12,500 µF; a 1 ms spike needs ~1000 µF. Hence 470–1000 µF for
stage 1, ~100 µF before the LM27762 for stage 2, never large capacitors on the LDO outputs. All
of it sits behind the soft-start switch because USB allows only about 10 µF directly on VBUS.

## Verifying without an oscilloscope

- **Oscillation:** idle supply current far above the datasheet value, a warm chip, or readings that
  change when wires are touched. The most useful check.
- **DC offset:** multimeter DC across the load.
- **Clipping:** step a 1 kHz (or 400 Hz, depending on the meter's AC bandwidth) tone up 1 dB at a
  time; the AC reading stops following.
- **Thumps, hum, hiss:** listen, cheap headphones first.
- Breadboards invite oscillation: 100 nF at the pins, short feedback/output wires, a few ohms in
  series with each output. Borrowing a scope for final checks is still worthwhile.

## Volume, line output, meters

- Volume is firmware gain (knob × host Feature Unit), so everything after the DAC follows it,
  including a future line output. A fixed-level line output would need analog volume or a second
  DAC; not planned. Line output itself is TBD.
- Meter levels come from the samples before the knob gain (tap before `attenuate()` in
  `fill_from_usb()`); host volume is already in the samples and can't be undone. Smoothing and PWM
  updates must not block USB/DMA servicing.
- Meter movements: select only with documented full-scale current, coil resistance and lamp
  voltage/current. LM358 is adequate as the PWM → current driver.

## Open items

1. M2 schematic for stage 1 (values, connector).
2. Soft-start switch part and power budget (M2 Part B); then raise `bMaxPower`.
3. Encoder part (detented vs smooth, no push switch) and the Feature Unit firmware. Mute button later.
4. Line output: yes/no.
5. Meter movements and lamps.
6. Stage 2 schematic after reading TIDA-01341 and both datasheets.

## Reference material

- [NJM4556A manufacturer specifications](https://www.nisshinbo-microdevices.co.jp/en/products/operational-amplifier/spec/?product=njm4556a)
- [OPA1622 product page](https://www.ti.com/product/OPA1622)
- [LM27762 product page](https://www.ti.com/product/LM27762)
- TI reference design TIDA-01341, "Hi-Fi Headphone Amplifier Power Supply"
- [MAX97220A-E datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX97220A-MAX97220E.pdf) (considered, dropped)
- [PCM5102A datasheet](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf)
