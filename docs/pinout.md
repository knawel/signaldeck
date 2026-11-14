# Pinout and Interconnects

> Breadboard prototype wiring, working since lab 7 (I2S) and lab 8 (USB audio). It may still change for the custom PCB. The firmware's pin numbers are in `firmware/rp2040/src/main.c`.

## RP2040-Zero -> PCM5102A

| RP2040 function | PCM5102A module | Purpose | GPIO |
|---|---|---|---|
| I2S BCLK | BCK / BCLK | Bit clock, 3.072 MHz | GP0 |
| I2S LRCLK | LCK / LRCK / WS | Left/right word clock, 48 kHz | GP1 (must be BCLK + 1: PIO side-set) |
| I2S DATA OUT | DIN | PCM audio data | GP2 |
| GND | GND | Common reference | GND |
| Power | VIN / VCC | DAC supply | RP2040-Zero 5V pin, on its own jumper |

No MCLK: the module's **SCK is tied to GND**, so the PCM5102A makes its own clock from BCK with its internal PLL.

### PCM5102A configuration pins

| Pin | Level | Meaning |
|---|---|---|
| FLT | L (GND) | normal-latency filter |
| DEMP (DMP) | L (GND) | de-emphasis off |
| XSMT (XMT) | H (3V3, never 5 V) | soft-mute off. Low = muted |
| FMT | L (GND) | I2S format (the firmware's PIO program is I2S, not left-justified) |

On modules with a header these come out as pins and must never float. On jumper versions they are solder pads on the back.

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
