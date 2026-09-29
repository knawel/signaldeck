# signaldeck Labbook

A hands-on path from "blink an LED" to "a DAC built from a GPIO and some resistors", before the PCM5102A module is added.

**How to use this book**
- Each lab has **Goal → Background → Wiring → Tasks → Checks → Notes**.
- Code skeletons live in `firmware/labs/NN-name/`. Replace the `TODO`s yourself and use the hints only when you're stuck.
- Fill in the **Notes** section of each lab as you go: what you measured, what surprised you, what broke. Future-you will thank you.
- A lab is done when every box in its **Checks** list is ticked.

## Lab index

| # | Lab | Status |
|---|---|---|
| 0 | Toolchain, hello, blink | ⬜ in progress |
| 1 | 1-bit square-wave tone | ⬜ |
| 2 | PWM DAC + RC low-pass filter | ⬜ |
| 3 | Sine synth (lookup table + phase accumulator) | ⬜ |
| 4 | Interrupt → DMA sample feeding | ⬜ |
| 5 | 8-bit R-2R ladder DAC | ⬜ |
| 6 | *(optional)* Sigma-delta output via PIO | ⬜ |
| 7 | PCM5102A over I2S | ⬜ |
| 8 | USB audio → PCM5102A (main firmware) | ⬜ |

---

## Prototype board: base layout

This base wiring stays in place for all labs. Each lab only adds parts to its own zone.

### RP2040-Zero pins

Top view, USB-C at the top. **Check this against the silkscreen on your board** before wiring. Clones sometimes differ.

```text
                 ┌──── USB-C ────┐
          5V  ●──┤               ├──● GP0
          GND ●──┤               ├──● GP1
          3V3 ●──┤   RP2040-Zero ├──● GP2
          GP29●──┤               ├──● GP3
          GP28●──┤   (GP16 =     ├──● GP4
          GP27●──┤   onboard     ├──● GP5
          GP26●──┤   RGB LED)    ├──● GP6
          GP15●──┤               ├──● GP7
          GP14●──┤               ├──● GP8
                 └──●──●──●──●──●┘
                    bottom edge: GP9 … GP13
```

> ⚠️ **Solder only the two long side headers for breadboard use.** The 5 bottom-edge pins (GP9–GP13) run *across* the breadboard strips, so on a breadboard they'd be shorted together. If you need them later, solder short wires to those pads instead.

### Breadboard layout

```text
   (+) 3V3 rail  ═══════════════════════════════════════════════════
   (−) GND rail  ═══════════════════════════════════════════════════
                   │    │
                   │    └────────────── GND pin
                   └─────────────────── 3V3 pin
          ┌────────────────┐
          │  RP2040-Zero   │   ← straddles the centre gap
          │  (USB-C faces  │
          │   the edge)    │
          └────────────────┘
   ─ ─ ─ ─ ─ ─ centre gap ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
   zone A: status LED  │ zone B: audio filter + 3.5 mm jack │ zone C: R-2R ladder
   (−) GND rail  ═══════════════════════════════════════════════════
   (+) 3V3 rail  ═══════════════════════════════════════════════════
```

Base wiring (do this once):
1. Put the RP2040-Zero near one end, straddling the centre gap, with the USB-C facing the board edge so the cable doesn't get in the way.
2. **3V3 pin → red (+) rail.** Only 3.3 V goes on the rails. Keep **5V off the rails** so nothing 3.3 V-only gets 5 V by accident.
3. **GND pin → blue (−) rail.** Bridge the top and bottom GND rails with a jumper, and the 3V3 rails too.
4. Put a **100 nF ceramic capacitor** across 3V3 and GND near the far end of the rails. This is local decoupling. It doesn't matter much today, but it becomes a habit for analog work.
5. Power comes from the USB-C cable. It must be a **data** cable, because you'll also flash and read `printf` over it.

### Pin plan for all labs

Only edge pins are used, so there's no need for the bottom pads.

| Pin | Use | Labs |
|---|---|---|
| GP0–GP7 | R-2R ladder bits 0 (LSB) … 7 (MSB), contiguous so one masked write sets all 8 bits | 5 |
| GP8 | UART1 TX debug output, for when USB is busy doing audio | 8 |
| GP14 / GP15 | Audio out L / R: PWM slice 7, channels A / B | 1–4, 6 |
| GP16 | Onboard WS2812 RGB LED (not on a header) | later |
| GP26 / GP27 / GP28 | ADC0–2: measure our own DAC outputs ("loopback") | 2+ |
| GP29 | External status LED | 0+ |
| GP9–GP13 | Bottom pads, unused on the breadboard. Main firmware's I2S placeholder is GP10–12, revisit in lab 7 | 7 |

---

## Lab 0 — Toolchain, hello, blink

### Goal
Prove the whole chain works (**edit → build → flash → run → observe**) with the simplest possible program: blink an LED and print a counter.

### Background
- **Cross-compiling.** Your Mac is ARM64, and the RP2040 is an ARM Cortex-M0+. They are different instruction sets and have no shared OS, so you need a separate compiler, `arm-none-eabi-gcc`. "none" means no operating system and "eabi" is the calling convention.
- **The Pico SDK** is a C library plus CMake scripts. CMake reads `CMakeLists.txt` and generates build instructions, and Ninja runs them. The VS Code extension's buttons are just shortcuts for these steps. `pico_stdlib` gives you GPIO, timing, and `printf`.
- **UF2 flashing.** Hold **BOOT** while plugging in USB and the RP2040 appears as a USB drive called `RPI-RP2`. Copy a `.uf2` file onto it and the board flashes itself and reboots.
- **`printf` over USB.** With `pico_enable_stdio_usb`, the board shows up as a serial port (`/dev/tty.usbmodem…`) after it boots.
- **GPIO output.** A pin set to output drives either 3.3 V (`1`) or 0 V (`0`). An LED needs a series resistor to limit current, which is Ohm's law:

  ```text
  I = (V_supply − V_LED) / R = (3.3 V − ~2.0 V) / 330 Ω ≈ 4 mA
  ```
  That's bright enough to see and well within what a GPIO can supply.

### Parts
- RP2040-Zero on the base layout (above)
- 1 × LED (any colour)
- 1 × 330 Ω resistor (anything from 220 Ω to 1 kΩ works; higher is dimmer)
- 1 jumper wire

### Wiring (zone A)

```text
  GP29 ──[ 330 Ω ]──►|── GND rail
                   anode  cathode
                  (long)  (short leg, flat side)
```

An LED only conducts one way. If it doesn't light, try flipping it before anything else.

### Tasks

**0.1 Install the toolchain** (one-time, with the VS Code extension)
1. In VS Code, install the **Raspberry Pi Pico** extension (publisher: Raspberry Pi).
2. Open the Pico panel from the sidebar and choose **Import Project**. Pick the folder `firmware/labs`, choose the **latest SDK 2.x**, and leave the other options at their defaults.
3. The extension downloads the ARM compiler, the SDK, CMake/Ninja and picotool into `~/.pico-sdk/`. It also adds a `== DO NOT EDIT ==` block to `firmware/labs/CMakeLists.txt` and creates a `.vscode/` folder. Commit both, and have a look at what it added so you know what changed.
   > ⚠️ Always open or import **`firmware/labs`**, never a single lab folder like `00-hello`. The lab folders aren't complete projects on their own: `project()` and `pico_sdk_init()` live in `labs/CMakeLists.txt`. If you open a lab folder by itself, `compile_commands.json` comes out empty and IntelliSense can't find `main.c`.

The board is set to `waveshare_rp2040_zero` in `CMakeLists.txt`. That tells the SDK the flash size and pin details for your board.

<details><summary>Alternative: command-line install without the extension</summary>

```sh
brew install --cask gcc-arm-embedded
git clone https://github.com/raspberrypi/pico-sdk ~/pico/pico-sdk
cd ~/pico/pico-sdk && git submodule update --init
echo 'export PICO_SDK_PATH=$HOME/pico/pico-sdk' >> ~/.zshrc
cd firmware/labs && cmake -S . -B build && cmake --build build
```
</details>

**0.2 Build the untouched skeleton** by clicking **Compile** in the status bar, so you know the build works before you change anything. You should end up with `build/00-hello/lab00_hello.uf2`.
- Read the build output once. You'll see CMake configuring, then `arm-none-eabi-gcc` compiling each file, then linking. That's the cross-compiler at work.

**0.3 Write the code.** Open `00-hello/main.c` and do TODO 1–3, then compile again.

**0.4 Flash.** Hold BOOT, plug in USB, release BOOT, then click **Run** in the status bar. It uses picotool. You can also drag the `.uf2` onto the `RPI-RP2` drive.
- Once your firmware has USB `printf` running, **Run** can usually reboot the board into BOOT mode by itself, so you won't need the button every time.

**0.5 Watch the output.** Open the **Serial Monitor** tab in the bottom panel, pick the `usbmodem…` port, and connect.

### Hints (read only if stuck)
<details><summary>TODO 1: pin setup</summary>

Look for `gpio_init()` and `gpio_set_dir()` in the SDK docs. The direction constant is `GPIO_OUT`.
</details>

<details><summary>TODO 2: blinking</summary>

`gpio_put(pin, value)` and `sleep_ms(ms)`.
</details>

<details><summary>TODO 3: printing a uint32_t</summary>

On this compiler `uint32_t` is an `unsigned long`, so `%u` gives a warning. The portable way is `#include <inttypes.h>` with `printf("blink %" PRIu32 "\n", count);`. The simple way is `%lu`. Try both and read the compiler warnings.
</details>

<details><summary>Nothing shows up in <code>screen</code>?</summary>

USB serial takes about a second to appear after boot, so the first prints can be lost before you connect. Add `sleep_ms(2000);` after `stdio_init_all()` while testing.
</details>

### Stretch challenges
- [ ] Print the system clock at boot: `clock_get_hz(clk_sys)` (needs `#include "hardware/clocks.h"`). What frequency is the CPU running at? You'll need this number in lab 2.
- [ ] Measure how long one `printf` takes, using `time_us_64()` before and after. Why does this matter for audio code?
- [ ] Drop `BLINK_MS` to 1 ms. What does the LED look like? Measure the pin with a multimeter on DC volts. What voltage do you read, and why? *(This is the core idea of lab 2.)*

### Checks
- [ ] `arm-none-eabi-gcc --version` works
- [ ] Skeleton builds and produces a `.uf2`
- [ ] LED blinks at ~2 Hz
- [ ] Counter prints in the serial terminal
- [ ] Compiles with **zero warnings**

### Notes
*Date:*

*Toolchain versions (gcc, SDK):*

*What went wrong and how I fixed it:*

*Measured voltage on GP29 at 1 ms blink:*

*Questions for next time:*

---

<!-- Starting a new lab: create NN-name/ with its own CMakeLists.txt, add add_subdirectory(NN-name)
     to labs/CMakeLists.txt, and add "NN-name/<executable>" to the "lab" input in .vscode/tasks.json
     so the Run button can flash it. -->

<!-- Template for the next lab — copy it below
## Lab N — Title

### Goal
### Background
### Parts
### Wiring
### Tasks
### Hints
### Stretch challenges
### Checks
### Notes
-->
