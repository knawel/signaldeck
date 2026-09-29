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
| 0 | Toolchain, hello, blink | ✅ done |
| 1 | 1-bit square-wave tone | ⬜ in progress |
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

## Lab 1 — 1-bit square-wave tone

### Goal
Make your first sound: switch one pin between 0 V and 3.3 V hundreds of times per second, and hear it. This is the crudest DAC possible, with just two output levels.

### Background
- **Frequency and period.** A tone of frequency *f* repeats every *T = 1/f* seconds. At 440 Hz, *T* ≈ 2 273 µs. A square wave spends half of *T* high and half low, so you wait *T/2* twice.
- **Integer division truncates.** `1000000 / 880` is `1136`, not `1136.36…`. The remainder is thrown away, so the tone you get is slightly off. How far off? That's TODO 3.
- **Why a square wave sounds buzzy.** Mathematically, a square wave is the fundamental frequency plus every **odd harmonic** (3*f*, 5*f*, 7*f*…), at amplitudes 1/3, 1/5, 1/7…. A pure sine tone has no harmonics. Removing them is what labs 2 and 3 are about.
- **Busy-waiting vs a hardware timer.** In part A, `sleep_us()` keeps the CPU occupied timing the pin, and anything else you add to the loop (like a `printf`) makes the timing wrong. In part B, a **hardware timer** raises an *interrupt* every half period. The CPU stops whatever it's doing, runs your small callback function, then carries on. You'll use this pattern for all real audio work.
- **Function pointers.** You pass `toggle_callback` itself (not its result) to the timer. In C, a function's name without `()` is its address. The SDK stores that address and calls the function later.
- **DC blocking.** The pin swings between 0 and 3.3 V, so on average it sits at **+1.65 V DC**. DC through a headphone driver does nothing useful: it pushes the membrane off-centre and heats the voice coil. A series capacitor passes the changing (AC) part and blocks the DC part.

### Parts
- 1 × 1 kΩ resistor (limits current, sets volume)
- 1 × 47 µF electrolytic capacitor, rated ≥ 6.3 V (DC blocking; anything from 10 µF up works)
- 3.5 mm stereo socket or breakout
- Cheap earbuds or a powered speaker's line input. **Don't use good headphones.**
- A phone with a free **tuner app**, to measure the pitch you produce

### Wiring (zone B)

```text
                            47 µF
  GP14 ──[ 1 kΩ ]────────(+)┤├(−)──┬──── jack TIP   (left)
                                    └──── jack RING  (right)  ← join both: sound in both ears
  GND rail ──────────────────────────────── jack SLEEVE
```

- **Electrolytic capacitors have polarity.** The **+** leg (longer leg; the stripe marks **−**) faces GP14, because that side sits at +1.65 V on average.
- **Rough volume:** earbuds are about 32 Ω, so the 1 kΩ resistor and the earbud form a voltage divider: 3.3 V × 32 / (1000 + 32) ≈ **0.1 V peak-to-peak** across the earbud. That's audible and safe, but square waves are harsh. **Start with the earbuds out of your ears, held near them.** Use 2.2 kΩ if it's too loud.
- **High-pass corner:** the capacitor and the resistance form a filter that blocks very low frequencies. *f = 1 / (2π · 1032 Ω · 47 µF)* ≈ 3.3 Hz, far below hearing (≈ 20 Hz). A 10 µF capacitor would give ≈ 15 Hz, also fine. A bigger capacitor just lets even lower frequencies through.
- **Start-up thump:** when the tone starts, the capacitor has to charge to the pin's +1.65 V average through the resistor. The time constant is *τ = R · C* ≈ 1032 Ω × 47 µF ≈ 50 ms, and you may hear it as a soft thump at start or after a reset. That's normal (10 µF would give ≈ 10 ms, a quieter thump). Real audio gear adds anti-pop circuits to hide it.

### Tasks

**1.1 Build the untouched skeleton.** Pick `01-square-tone/lab01_square_tone` when you click Run. You'll see one warning: `unused variable 'half_period_us'`. That's the compiler noticing your unfinished TODO, and it goes away once you use the variable.

**1.2 Part A: busy-wait tone** (`USE_TIMER 0`)
- TODO 1–4: set up the pin, compute the half period, print what you'll get, and toggle it in the loop.
- Listen, then measure the pitch with the tuner app. Is it 440 Hz?

**1.3 Break it on purpose.** Add a `printf` inside the part A loop. Listen: what happens to the pitch and to the sound quality? Measure again. *(Remember lab 0: one `printf` ≈ 77 µs.)*

**1.4 Part B: timer tone** (`USE_TIMER 1`)
- TODO 6–8: flip the pin in the callback, start the repeating timer, and print once per second from the main loop.
- Does the `printf` in the main loop still disturb the tone? Why or why not?

### Hints (read only if stuck)
<details><summary>TODO 2: half period</summary>

1 s = 1 000 000 µs. A full period is `1000000 / TONE_HZ` µs, and you need half of that. Is it better to write `1000000 / TONE_HZ / 2` or `1000000 / (2 * TONE_HZ)`? Try some frequencies where the order matters for truncation.
</details>

<details><summary>TODO 3: the frequency you actually get</summary>

Turn it around: actual *f* = 1 000 000 / (2 × half_period_us). Printing it with decimals needs `float` or a trick. Try printing it in **milli-hertz** using integers only: `1000000000 / (2 * half_period_us)`.
</details>

<details><summary>TODO 6: flipping a pin</summary>

Two ways: `gpio_put(AUDIO_PIN, !gpio_get(AUDIO_PIN));` reads the pin and writes the opposite, or `gpio_xor_mask(1u << AUDIO_PIN);` flips it in one hardware operation. What does `1u << 14` evaluate to in binary?
</details>

<details><summary>TODO 7: starting the timer</summary>

`add_repeating_timer_us(delay_us, callback, user_data, &timer)`. Use `NULL` for user_data. The **sign** of `delay_us` matters: a negative value means "measured from the start of one callback to the start of the next", which is what you want for a steady tone. Check the SDK docs for what a positive value means.

`timer` must stay alive while the timer runs. Declaring it in `main()`, which never returns, does that.
</details>

<details><summary>TODO 8: once per second without sleep_ms?</summary>

`sleep_ms(1000)` is fine here. The timer interrupt keeps toggling the pin while `main` sleeps. That's the whole point of part B.
</details>

### Stretch challenges
- [ ] **Hearing range:** try 50 Hz, 1 kHz, 10 kHz, 15 kHz, 18 kHz. Where does it stop being audible for you? What happens to `half_period_us` accuracy at high frequencies?
- [ ] **Duty cycle:** make the high part 25 % of the period instead of 50 %. How does it sound? Measure the pin with a multimeter at 50 % and at 25 %. *(Preview of lab 2.)*
- [ ] **Melody:** define `struct note { uint32_t hz; uint32_t ms; };` and an array of notes, then play them in a loop. Use 0 Hz for a rest. Arrays and structs are your C concepts here.

### Checks
- [ ] Hear a steady tone in part A
- [ ] Tuner app shows ≈ 440 Hz. Record the exact reading in Notes.
- [ ] Printed actual frequency matches what you calculated by hand
- [ ] Explain what the `printf` in the part A loop did to the sound
- [ ] Part B: tone stays steady while the main loop prints
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:*

*Calculated half period / actual frequency:*

*Tuner app reading (part A / with printf / part B):*

*Highest frequency I can hear:*

*Capacitor used (value, voltage rating) and how its polarity was marked:*

*Did I hear the start-up thump?*

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
