# Analog expansion — discussion notes

Updated: 2026-11-15. **Analysis only: no amplifier, supply, circuit, meter or shopping list has been approved.**
These notes capture the current discussion; they do not replace the existing architecture,
pinout, BOM or decision log. Older planning choices in those documents remain unchanged.

## Working baseline

- RP2040-Zero -> I2S -> PCM5102A module; macOS identifies “signaldeck USB DAC”.
- Playback works, with 24-bit/48kHz stereo, PIO/DMA and asynchronous USB feedback.
- GP0 = BCK, GP1 = LRCK, GP2 = DIN. SCK/FLT/DEMP/FMT grounded; XSMT at 3V3.
- The module's VIN is powered from the board 5V pin; this is not the bare DAC's supply pin.
- Firmware applies fixed attenuation using `OUTPUT_SHIFT=4` (approximately -24dB).
- Variable volume, encoder input, meter processing and analog power sequencing are not implemented.
- USB configuration currently declares 100mA. Additional hardware requires a measured power
  budget and an appropriate USB power declaration/enable sequence.

## Goals and preferences under discussion

- Headphone output for ATH-M40x (35 ohms), DT770 Pro (80 ohms) and generic PC headphones.
- Initial request included full 16-ohm support. The user is now considering a 32-ohm rated
  minimum, with accidental 16-ohm connection tolerated at reduced output. This is not yet confirmed.
- Separate line output for active PC speakers, which have their own amplifier.
- Prefer an endless rotary encoder with push-to-mute and firmware volume; avoid fixed
  MIN/MAX position markings. Detailed control behavior is undecided.
- Two warm-lit physical needle meters, with levels calculated from PCM before local volume
  control and smooth VU-like movement. Exact movements and driver circuit remain undecided.
- Aim for USB power and an external, mechanically separate USB connector.
- Complete the electronics before enclosure/custom PCB design.
- User does not have an oscilloscope; assembly and verification effort must influence selection.

## Amplifier options

| Option | Reasons to consider it | Unresolved issues |
|---|---|---|
| NJM4556A DIP-8 | Easy to handle and solder; suitable for a hands-on analog build | Supply topology, gain, clean output into 32/80 ohms, overload behavior, noise and startup transients |
| MAX97220A | Single 5V input with internal negative-rail charge pump; published 16-ohm measurements | Tiny IC package; an affordable, documented assembled module has not been established |
| Other assembled headphone module | Could reduce layout and verification uncertainty | Must check schematic, supply, gain, output impedance and headphone-load ratings |

MAX97220A was investigated for power-supply convenience, not a demonstrated sound-quality
advantage over NJM4556A. The documented MAX97220AEVKIT+ listing showed about EUR94;
that makes it an expensive reference option, not a purchase recommendation.

NJM4556A's advertised 70mA output drive is not a guarantee of clean operation into every
headphone load. Lower impedance increases current demand at the same voltage:

| Output | 32-ohm load | 16-ohm load |
|---|---:|---:|
| 1Vrms | 31mA RMS / 44mA peak | 63mA RMS / 88mA peak |
| 2Vrms | 63mA RMS / 88mA peak | 125mA RMS / 177mA peak |

These are Ohm's-law calculations, not amplifier performance predictions. A possible requirement
is clean rated output into 32 ohms and above, with no damage from accidental 16-ohm operation.
At high levels the lower load may cause clipping, heating or current limiting. Harmless overload
must be verified for the complete circuit; it cannot be assumed from the chip name.

## NJM4556A supply and build effort

The amplifier circuit is relatively straightforward; a quiet USB-derived supply and reliable
startup behavior add difficulty. Two approaches remain open:

- A ready-made regulated bipolar converter, with approximately +/-5V as a starting point to
  investigate. Check output current, ripple, light-load regulation, filtering and USB input draw.
- A single-supply biased circuit, which requires a suitable signal reference and headphone
  output coupling capacitors. A simple resistor divider is not a headphone return-current sink.

Neither supply approach nor any converter model has been selected. There is no approved
NJM4556A schematic or component-value list in this analysis.

A multimeter and dummy loads can check rails, DC offset, supply current and approximate output
voltage where the meter's AC bandwidth permits. They cannot establish absence of ultrasonic
oscillation or reliably capture clipping/startup spikes. Borrowing a scope for final checks would
make a custom build easier to assess. An assembled module reduces assembly uncertainty but
still needs sensible power, wiring and load checks.

## Controls, outputs and meters: consequences to retain

- Digital volume before a single PCM5102A changes both its line and headphone outputs.
  Independent fixed-level line output needs a different headphone-volume arrangement or
  a separate DAC path. Desired line-output behavior has not been confirmed.
- Pre-volume PCM metering can remain active during local mute. Host/app volume is already
  reflected in received samples; the device cannot recover the original source level.
- The sample tap can be placed before `attenuate()` in `fill_from_usb()`. Meter smoothing and
  PWM updates should not block USB/DMA servicing. No new GPIO assignments are reserved.
- Select meter movements only with documented full-scale current, coil resistance and
  illumination voltage/current. Backlights may use much more power than the movements.
- Filtered PWM with an op-amp/transistor current driver is a possible approach. LM358 could
  serve in this low-frequency control role; it is not the proposed headphone output stage.
- No meter model, lighting converter, driver values, socket-priority wiring or LOW/HIGH
  range switch is selected. The previous detailed circuit and 445mA budget were provisional
  and have been removed rather than retained as established design facts.
- A short panel-mount USB data harness may avoid desoldering the RP2040 connector. Exact
  connector/harness choice, power distribution and any power switching remain to be designed.

## Next decisions

1. Confirm the rated headphone-load range and desired output level.
2. Choose custom NJM4556A versus a documented assembled amplifier, considering cost and
   available measurement equipment.
3. Choose the supply with that amplifier; then develop one reviewable schematic and power budget.
4. Resolve shared versus independent line/headphone volume; select meter hardware afterward.
5. Update architecture, pinout and BOM only when the relevant choices are agreed.

## Reference material already consulted

- [NJM4556A manufacturer specifications](https://www.nisshinbo-microdevices.co.jp/en/products/operational-amplifier/spec/?product=njm4556a)
- [MAX97220A-E datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX97220A-MAX97220E.pdf)
- [MAX97220A evaluation-board documentation](https://www.analog.com/media/en/technical-documentation/data-sheets/max97220aevkit.pdf)
- [PCM5102A datasheet](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf)

No firmware, physical wiring or purchases changed during this analysis.
