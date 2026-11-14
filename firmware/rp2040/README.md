# signaldeck firmware (RP2040)

USB sound card: UAC2, 48 kHz, 24-bit stereo, asynchronous with a feedback endpoint, playing
through a PCM5102A over I2S. Promoted from lab 8 (`firmware/labs/08-usb-audio`, Part C);
the background for every part is in `firmware/labs/labbook2.md`, labs 7 and 8.

## Hardware

RP2040-Zero → PCM5102A module: GP0 → BCK, GP1 → LCK (LRCK), GP2 → DIN, 5V → VIN, GND, SCK to GND,
FMT set to I2S.

## Files

| File | What's in it |
|---|---|
| `src/main.c` | I2S + DMA ping-pong, TinyUSB callbacks, USB → slot unpacking, feedback controller |
| `src/i2s.pio` | the I2S PIO program |
| `src/tusb_config.h` | TinyUSB settings: classes, stream format, buffer sizes |
| `src/usb_descriptors.c/.h` | device and configuration descriptors (audio + serial + picotool reset) |

## Build and flash

Open this folder (`firmware/rp2040`) in VS Code with the Raspberry Pi Pico extension, build, then
run the **Run Project** task. The device has its own USB IDs (`0xCAFE:0x4008`), so the task passes
`--vid 0xcafe --pid 0x4008` to picotool. From a shell:

```sh
picotool load -x build/signaldeck.elf -f --vid 0xcafe --pid 0x4008
```

If the board is running something else (a lab), hold BOOT while plugging it in once.

## Status line (serial port)

`playing FIFO 295.0 frames | feedback 48.01450 | short 0, overflow 0, late 0`

- **FIFO**: averaged frames waiting in TinyUSB's FIFO. A P controller (KP 8) holds it below the
  392-frame target by an amount set by the crystal mismatch between host and board.
- **short / overflow / late** should stay 0 while playing.

## Known issues and next steps

- `tud_audio_set_itf_close_EP_cb()` works around a TinyUSB 0.18 bug on the RP2040 that panicked
  on every resume after a pause ("ep 01 was already available"). Re-check when the SDK updates.
- No volume control: `OUTPUT_SHIFT` attenuates digitally (−24 dB). The plan is an analog knob.
- Only 48 kHz. 44.1 kHz, a PI controller and Windows (16.16 feedback) are lab 8's stretch tasks.
