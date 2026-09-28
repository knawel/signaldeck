# Pinout and Interconnects

> **Important:** GPIO numbers are not frozen yet. Verify the exact RP2040-Zero pinout and the selected PCM5102A module before wiring hardware.

## RP2040-Zero -> PCM5102A

Planned logical signals:

| RP2040 function | PCM5102A module | Purpose | GPIO |
|---|---|---|---|
| I2S BCLK | BCK / BCLK | Bit clock | TBD |
| I2S LRCLK | LCK / LRCK / WS | Left/right word clock | TBD |
| I2S DATA OUT | DIN | PCM audio data | TBD |
| GND | GND | Common reference | GND |
| Power | VIN / VCC as required by module | DAC supply | TBD after module inspection |

MCLK is not planned for the initial configuration if the selected PCM5102A module supports operation without it.

## DAC analog output

```text
PCM5102A L OUT ----+----> LEFT meter-driver input (high impedance)
                   |
                   +----> volume potentiometer LEFT input

PCM5102A R OUT ----+----> RIGHT meter-driver input (high impedance)
                   |
                   +----> volume potentiometer RIGHT input

PCM5102A GND ------------> analog ground/reference
```

## Volume potentiometer

Planned component: dual-gang A10K.

For each channel:

```text
DAC output ---- potentiometer end
                   |
                 wiper ----> headphone amplifier input
                   |
GND ---------- potentiometer other end
```

Confirm orientation experimentally so clockwise rotation increases volume.

## Headphone outputs

Both connectors are stereo TRS.

```text
Tip   = Left
Ring  = Right
Sleeve = Ground
```

Whether the 3.5 mm and 6.35 mm sockets remain active simultaneously or use switched contacts is TBD.

## Power domains

Current conceptual domains:

- USB VBUS / +5 V
- RP2040 regulated 3.3 V
- switched audio power
- analog ground/reference

Do not assume that every module's `5V`, `VIN`, `VCC`, or `3V3` pin is interchangeable. Confirm each board before connection.

## Future microphone

Reserve conceptually, but no GPIO/interface assignment yet.
