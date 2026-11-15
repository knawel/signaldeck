# Pinout and Interconnects

> Hardware revision **P1**: breadboard prototype wiring, working since lab 7 (I2S) and lab 8 (USB audio). It may still change for later revisions. The firmware's pin numbers are in `firmware/rp2040/src/main.c`.

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
| XSMT (XMT) | H (3V3, never 5 V) | soft-mute off. Low = muted. Planned to move to a GPIO for power sequencing |
| FMT | L (GND) | I2S format (the firmware's PIO program is I2S, not left-justified) |

On modules with a header these come out as pins and must never float. On jumper versions they are solder pads on the back.

## DAC analog output

```text
PCM5102A L OUT ----+----> headphone amplifier L input (input impedance >= 10 kOhm)
                   +----> line output L (TBD)

PCM5102A R OUT ----+----> headphone amplifier R input
                   +----> line output R (TBD)

PCM5102A GND ------------> audio ground (to the amplifier module)
```

There is no analog volume control or meter tap: volume and meter levels are computed in firmware
(`architecture.md`, sections 5 and 8).

## Amplifier module connector

The same connector for stage 1 (NJM4556A, breadboard) and stage 2 (OPA1622 board):

| Pin | Signal | Stage 1 | Stage 2 |
|---|---|---|---|
| 1 | +5 V switched (after the soft-start switch) | amp supply | LM27762 input |
| 2 | GND | | |
| 3 | L in (DAC L OUT) | | |
| 4 | R in (DAC R OUT) | | |
| 5 | Audio ground (DAC GND) | | |
| 6 | AMP_EN (GPIO) | not used | LM27762 enable |
| 7 | AMP_MUTE (GPIO) | relay driver transistor | OPA1622 shutdown, possibly level-shifted |
| 8 | AMP_PG (optional, input to RP2040) | not used | LM27762 power-good |

Connector type and pin order are provisional until the stage 1 build.

## Planned GPIO functions (not yet assigned)

Rotary encoder A and B; 2 meter PWM outputs; AUDIO_EN (soft-start switch); AMP_EN;
AMP_MUTE; AMP_PG; DAC XSMT; power toggle input; headphone-detect inputs (socket switch contacts);
status LEDs; later a mute button. About 15 pins in total, which fits the RP2040-Zero. Pins get assigned in the lab that
first uses them.

## Headphone outputs

Both connectors are stereo TRS, with switch contacts (for headphone detection).

```text
Tip   = Left
Ring  = Right
Sleeve = Ground
```

Whether the 3.5 mm and 6.35 mm sockets are active simultaneously is TBD.

## Power domains

- USB VBUS / +5 V: RP2040-Zero and PCM5102A module VIN, always on while USB is connected
- RP2040 regulated 3.3 V (logic levels, XSMT)
- switched 5 V audio rail (soft-start switch): amplifier, meter drivers, meter lamps
- stage 2 only: ±4.5 V from the LM27762 on the amp board
- audio ground/reference

Do not assume that every module's `5V`, `VIN`, `VCC`, or `3V3` pin is interchangeable. Confirm each board before connection.

## Future microphone

Reserve conceptually, but no GPIO/interface assignment yet.
