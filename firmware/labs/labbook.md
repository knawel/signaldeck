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
| 1 | 1-bit square-wave tone | ✅ done |
| 2 | PWM DAC + RC low-pass filter | ✅ done |
| 3 | Sine synth (lookup table + phase accumulator) | ⬜ in progress |
| 4 | Interrupt → DMA sample feeding | ⬜ next |
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

## Lab 2 — PWM DAC + RC low-pass filter

### Goal
Get **many output levels from one digital pin**. You'll switch GP14 on and off hundreds of thousands of times per second, smooth the result with a resistor and capacitor, and end up with a voltage you can set anywhere between 0 and 3.3 V. Then you'll change that voltage 20 000 times a second and hear a triangle wave. This is your first real DAC.

### Background
- **The core idea: averaging.** You've already seen this twice: the LED at a 1 ms blink in lab 0 and the 25 % duty stretch in lab 1. A pin that is high for a fraction *D* of the time (the **duty cycle**) has an average voltage of

  ```text
  V_avg = D × 3.3 V        (D = 0.25 → 0.83 V, D = 0.5 → 1.65 V, …)
  ```
  If you switch fast enough and then throw away everything except the average, you have an analog output. That's **PWM**: pulse-width modulation.

- **The RP2040 PWM hardware.** You don't toggle the pin from code any more. The chip has 8 PWM **slices** with 2 channels (A/B) each. Every slice has a 16-bit counter that counts `0, 1, … TOP`, then wraps back to 0. A channel's pin is **high while `counter < level`**, where *level* is the compare value you set.

  ```text
  counter:  0 1 2 3 4 5 6 7 | 0 1 2 3 4 5 6 7 |     TOP = 7 → 8 steps per period
  level=3:  ▔ ▔ ▔ _ _ _ _ _ | ▔ ▔ ▔ _ _ _ _ _ |     duty = 3/8
  ```
  - duty = level / (TOP + 1). With TOP = 255 you get 256 steps, which is **8 bits of resolution**. level = 0 is always low. level = 256 (more than TOP) is always high.
  - f_PWM = f_sys / (TOP + 1), before any clock divider.
  - GPIO → slice: slice = (gpio ÷ 2) mod 8, and even pins are channel A. GP14 is **slice 7, channel A**, and GP15 is slice 7 B. That's why the pin plan put audio there.
  - The compare value is **double-buffered**: a new level takes effect at the next wrap, never halfway through a period. You can change it whenever you like without making a glitch.

- **Resolution vs frequency: the PWM trade-off.** More levels means more counter steps per period, and so a slower period. You measured f_sys in the lab 0 stretch: **125 MHz**. That's the SDK's default for the RP2040. It only runs at 200 MHz if `PICO_USE_FASTEST_SUPPORTED_CLOCK` is set, which your board doesn't do. The code computes f_PWM from `clock_get_hz()`, so trust what it prints:

  | TOP | Levels (bits) | f_PWM @ 125 MHz (yours) | f_PWM @ 200 MHz |
  |---|---|---|---|
  | 255 | 256 (8) | 488 kHz | 781 kHz |
  | 1023 | 1024 (10) | 122 kHz | 195 kHz |
  | 4095 | 4096 (12) | 30.5 kHz | 48.8 kHz |
  | 65535 | 65536 (16) | 1.9 kHz ← you'd *hear* the switching | 3.05 kHz |

  The carrier (the switching frequency) has to sit far above the audio so a simple filter can separate the two. That's why this lab uses 8 bits. The PCM5102A gets 24 bits out of 3.3 V in a completely different way (lab 7).

- **The RC low-pass filter.** A resistor feeding a capacitor passes slow changes and smooths out fast ones. It's the same R and C as the DC blocker in lab 1, swapped around: there the capacitor was in series, and here it goes to ground.

  ```text
  f_c = 1 / (2π · R1 · C1) = 1 / (2π · 1 kΩ · 100 nF) ≈ 1.6 kHz
  ```
  - Above *f_c* the output falls by **20 dB per decade** (×10 in frequency means ÷10 in amplitude). That's a *first-order* filter.
  - The carrier at 488 kHz is ~305 × *f_c*, so it's attenuated about 305×, or −50 dB.
  - A 440 Hz tone loses only ~4 % (−0.3 dB). A 5 kHz tone loses ~70 % (−10 dB). **A lower f_c means less ripple but duller treble.** Every PWM DAC has this tension.
  - **Ripple**, the carrier that survives, is worst at 50 % duty. When RC is much larger than the PWM period *T*:

    ```text
    ripple_pp ≈ 3.3 V · D(1−D) · T / (R1·C1)
              = 3.3 · 0.25 · 2.05 µs / 100 µs ≈ 17 mV     (125 MHz, TOP = 255)
    ```
    One 8-bit step is 3.3 V / 256 ≈ 13 mV, so the ripple is **about one step**: slightly more at 50 % duty, less towards 0 % and 100 %. The filter is roughly matched to the resolution, but only just. At 200 MHz the period is shorter and the ripple drops to ≈ 11 mV, under one step. Keep this comparison in mind for the stretch challenges.

- **Don't load the filter.** Earbuds are about 32 Ω. Connect them straight to the filter output and they swamp C1, turning R1 and the earbud into a plain 1 kΩ : 32 Ω divider. The filter then effectively disappears (*f_c* jumps to ~50 kHz). R2 = 1 kΩ keeps the load at about 1 kΩ: the filter still works (*f_c* ≈ 3 kHz with the load attached), and the earbud level stays safe at about half of lab 1's. In the final device, a headphone amplifier with a high input impedance does this job.

- **Now it's a real DAC.** In part B the level changes **20 000 times per second**. Two numbers describe any DAC: **resolution** (here 8 bits = 256 levels) and **sample rate** (here 20 kHz). A CD is 16-bit / 44.1 kHz, and this project targets 24-bit / 48 kHz. A sample rate *f_s* can represent tones up to *f_s / 2* (the **Nyquist** limit, which comes back in lab 3).

- **Triangle vs square.** A triangle wave has the same odd harmonics as a square (3*f*, 5*f*, 7*f*…) but at **1/9, 1/25, 1/49…** instead of 1/3, 1/5, 1/7. The harmonics fall off much faster, so it sounds noticeably softer and rounder. It's halfway to a sine.

- **ADC loopback.** GP26 is also ADC input 0, a 12-bit converter: `0 … 4095` maps to `0 … 3.3 V`. Wire it to the filter output and the chip can **measure its own DAC**, with no instrument needed. One reading is noisy, so average several.

### Parts
- Everything from lab 1 (1 kΩ, 47 µF, jack, earbuds)
- 1 × extra 1 kΩ resistor (R2)
- 1 × 100 nF ceramic capacitor (C1). It's marked **"104"** (10 × 10⁴ pF). Ceramics have **no polarity**.
- Multimeter
- *(Optional)* oscilloscope, if you have access to one

### Wiring (zone B)
Rebuild lab 1's circuit: the lab 1 resistor becomes R1, and you add C1, R2 and the ADC wire.

```text
            R1 1 kΩ      FILT                    47 µF        R2 1 kΩ
  GP14 ───[ 1 kΩ ]───────┬──────────┬───────(+)┤├(−)────────[ 1 kΩ ]──┬── jack TIP
                         │          │                                  └── jack RING
                    C1 ═══ 100 nF   └────── GP26 (ADC0)
                         │
  GND rail ──────────────┴───────────────────────────────────────────────── jack SLEEVE
```

- **FILT** is the node to measure: multimeter (DC volts) between FILT and GND.
- Keep C1's legs short and put its ground leg straight into the GND rail. The filter can only be as good as its ground.
- The 47 µF still blocks DC. Its **+** leg now faces FILT, which sits at the average voltage.
- **The ADC pin must never see more than 3.3 V.** FILT can't go higher than the GPIO, so this wiring is safe. Remember this rule in later labs.

### Tasks

**2.1 Build the untouched skeleton.** Pick `02-pwm-dac/lab02_pwm_dac` when you click Run. It should build with `unused` warnings until you finish the TODOs: `read_filt_mv` in part A, and `i`, `timer`, `sample_callback` in part B.

**2.2 Part A: DC levels** (`PART_B 0`)
- TODO 1–3: set up PWM on GP14, print f_sys and f_PWM, and set up the ADC. Check that the printed f_PWM matches the table above.
- TODO 4: `read_filt_mv()`.
- TODO 5: step through 0 / 25 / 50 / 75 / 100 %. For each step, print the level, the expected mV and the measured mV.
- Check each step with the multimeter too, and fill in the table in Notes.
- **Question:** R1·C1 is only 100 µs, so FILT should settle in ~0.5 ms (5τ). Why do the first readings after each step still drift with the earbuds plugged in? *(Hint: which capacitor in the circuit is biggest, and what is it charging through?)*

**2.3 Break it on purpose: remove C1.** Pull C1 out, set `ADC_SAMPLES` to 1 and reflash.
- What does the ADC print now? What does the **multimeter** show? One of them still reads roughly the right average. Why that one? *(A multimeter's DC range averages over many milliseconds. The ADC takes a snapshot in about 2 µs.)*
- Put C1 back and set `ADC_SAMPLES` back to 16.

**2.4 Part B: triangle tone** (`PART_B 1`)
- TODO 6–8: generate a triangle in the timer callback, print the actual tone frequency, and start the timer at `SAMPLE_RATE_HZ`.
- Listen, earbuds out of your ears first. Compare it with lab 1's square wave at the same pitch. What's different?
- Measure the pitch with the tuner app. Does it match what TODO 7 printed?
- The main loop prints FILT once a second. What average do you expect for a triangle from 0 to AMPLITUDE? Does the ADC agree?

### Hints (read only if stuck)
<details><summary>TODO 1: PWM setup</summary>

Four calls:
- `gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM)`: the pin now listens to the PWM slice instead of `gpio_put`.
- `uint slice = pwm_gpio_to_slice_num(AUDIO_PIN);`: work it out by hand first. Is it 7?
- `pwm_set_wrap(slice, PWM_TOP)`
- `pwm_set_enabled(slice, true)`

The clock divider defaults to 1, so the counter runs at the full f_sys.
</details>

<details><summary>TODO 2: PWM frequency</summary>

`clock_get_hz(clk_sys) / PWM_LEVELS`. Why `PWM_LEVELS` (TOP + 1) and not `PWM_TOP`? Count the states in the diagram in Background.
</details>

<details><summary>TODO 3: ADC setup</summary>

`adc_init()`, then `adc_gpio_init(ADC_PIN)` (disables the pin's digital circuitry so it doesn't disturb the analog signal), then `adc_select_input(ADC_INPUT)`. GP26 → input 0, GP27 → 1, GP28 → 2.
</details>

<details><summary>TODO 4: averaging without overflow</summary>

```c
uint32_t sum = 0;
for (int n = 0; n < ADC_SAMPLES; n++) sum += adc_read();
return sum * VREF_MV / (ADC_SAMPLES * 4096);
```
Check the worst case: 16 × 4095 × 3300 ≈ 216 million. That fits in a `uint32_t` (max ≈ 4.29 billion). Why multiply *before* dividing? Try `sum / ADC_SAMPLES * VREF_MV / 4096` with a small reading and compare.
</details>

<details><summary>TODO 5: one step</summary>

`pwm_set_gpio_level(AUDIO_PIN, levels[k])`, then `sleep_ms(2000)` (long enough for the multimeter too), then expected mV = `levels[k] * VREF_MV / PWM_LEVELS`, then print both.
</details>

<details><summary>TODO 6: triangle</summary>

```c
uint32_t half = samples_per_period / 2;
uint32_t level = (i < half) ? AMPLITUDE * i / half
                            : AMPLITUDE * (samples_per_period - i) / half;
pwm_set_gpio_level(AUDIO_PIN, level);
if (++i >= samples_per_period) i = 0;
```
- 45 samples per period is odd. What does the first sample of the downward half compute to? Why is it harmless here? *(What does a level above PWM_TOP mean?)*
- Division inside an interrupt? The Cortex-M0+ has **no divide instruction**, but the RP2040 adds a hardware divider, and the SDK routes `/` to it automatically. It takes ~8 cycles, so it's fine here.
</details>

<details><summary>TODO 7: actual frequency</summary>

20 000 / 440 = 45.45…, which truncates to 45 samples. So the actual tone is 20 000 / 45 ≈ 444.4 Hz, about **17 cents sharp**. Lab 1's half-period truncation gave 440.1 Hz (under 1 cent off). This error is far bigger because 45 steps per period is much coarser than 1136 µs. Lab 3's *phase accumulator* fixes this properly.
</details>

<details><summary>TODO 8: 20 000 times a second</summary>

Same as lab 1: `add_repeating_timer_us(-(1000000 / SAMPLE_RATE_HZ), sample_callback, NULL, &timer)`. That's one interrupt every 50 µs. At 125 MHz that's 6 250 CPU cycles per sample, so there's plenty of room. Lab 4 hands this job to DMA so the CPU gets all of it back.
</details>

### Stretch challenges
- [ ] **Hear the carrier:** add `pwm_set_clkdiv(slice, 100.0f)` in part B. f_PWM drops to ~4.9 kHz (at 125 MHz), well inside hearing and far too close to the audio for the filter. What do you hear? What did the extra noise tell you about why the carrier has to be ultrasonic?
- [ ] **Resolution vs ripple:** set `PWM_TOP` to 4095 (12-bit). Use the ripple formula to calculate the new ripple and compare it with the new step size (3.3 V / 4096 ≈ 0.8 mV). Was that worth 4 extra bits? *(If you have a scope, look at FILT at 50 % duty with both settings.)*
- [ ] **Volume = fewer bits:** in part B, set `AMPLITUDE` to 64, then 16, then 4. The tone gets quieter, and it gets *grittier*, because a triangle with only 4 steps is a staircase. You're hearing **quantization noise**. This is why digital volume controls lose resolution and why the project uses an analog pot.
- [ ] **Second-order filter:** add a second R–C stage (another 1 kΩ + 100 nF) after C1. Each stage adds another −20 dB/decade. What happens to the carrier, and what happens to a 5 kHz tone?
- [ ] **Phase-correct PWM:** `pwm_set_phase_correct(slice, true)` makes the counter count up *and* down. What happens to f_PWM? Look it up in the RP2040 datasheet, section "PWM", to see why it exists.
- [ ] **Stereo:** separate the jack's TIP and RING, build a second filter on GP15 (slice 7 **B**), and play a different pitch in each ear from the same callback.

### Checks
- [ ] Printed f_PWM matches the table for your f_sys
- [ ] Multimeter on FILT reads ≈ 0 / 0.83 / 1.65 / 2.48 / 3.3 V for the five steps
- [ ] ADC readings agree with the multimeter to within ~50 mV
- [ ] Explain what removing C1 did to the ADC and why the multimeter didn't care
- [ ] Hear a triangle tone in part B and describe how it differs from lab 1's square
- [ ] Tuner reading matches the frequency printed by TODO 7
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:*

*f_sys / f_PWM printed:*

*Levels measured:*

| Level | Duty | Expected mV | Multimeter mV | ADC mV |
|---|---|---|---|---|
| 0 | 0 % | 0 | | |
| 64 | 25 % | 825 | | |
| 128 | 50 % | 1650 | | |
| 192 | 75 % | 2475 | | |
| 256 | 100 % | 3300 | | |

*Actual 3V3 rail voltage (multimeter):*

*With C1 removed, ADC showed / multimeter showed:*

*Why the readings drifted after each step (answer to the 2.2 question):*

*Triangle vs square, how it sounded:*

*Tuner app reading (part B):*

*Lowest AMPLITUDE that still sounds like a clean tone:*

*Questions for next time:*

---

## Lab 3 — Sine synth: lookup table + phase accumulator

### Goal
Play a **pure sine tone** at **any** frequency, accurate to millionths of a hertz, spending only a few CPU cycles per sample. Then sweep the frequency upwards and *hear* the Nyquist limit.

### Background
- **Why a sine?** Square (lab 1) and triangle (lab 2) waves are a fundamental plus harmonics. A sine is *only* the fundamental, the purest tone there is. Every other sound can be built from sines, so a sine generator is the "hello world" of audio DSP.

- **Why a lookup table?** `sinf()` on the Cortex-M0+ is a slow software routine (no floating-point hardware, remember lab 0). You can't afford it 20 000 times a second in an interrupt. So you compute **one period** of the sine **once** at startup, store it in an array, and just *read* from it later:

  ```text
  n:       0     64    128    192    255
  angle:   0°    90°   180°   270°   ~359°
  value:   128   255   128    1      125       value = 128 + 127·sin(angle)
  ```
  The sine swings ±127 around the middle PWM level, so the output sits at 1.65 V ± 1.64 V: the full range of the 8-bit PWM DAC from lab 2.

- **The phase accumulator: the key idea of this lab.** Picture the table as a clock face with 256 positions. Each sample, you step forward a fixed amount and read the value where you land. **Bigger steps mean going round faster, so a higher frequency.**

  A 256-position clock only allows whole steps, which gives coarse frequencies (lab 2's 444.4 Hz problem). The trick is to keep the position with far more precision than the table has. Use a whole `uint32_t` as the phase:

  ```text
   phase (32 bits):  [ 8-bit table index | 24 bits of "fraction" ]
                       ↑ phase >> 24        ↑ keeps the fine position between table entries
  ```
  - `0 … 2³² − 1` is one full circle. Adding `phase_inc` every sample moves around it.
  - **Overflow is a feature:** when `phase` passes 2³² − 1, unsigned arithmetic wraps back to 0. That's exactly "going round the circle again", for free. (Unsigned wrap-around bit you in lab 1. Here it's what you want.)
  - The table index is just the **top 8 bits**: `phase >> 24`.

- **Tuning word.** To get *f* Hz at sample rate *f_s*, the phase must go round *f* times per second, i.e. advance 2³² · *f* / *f_s* per sample:

  ```text
  phase_inc = f × 2³² / f_s = 440 × 4 294 967 296 / 20 000 = 94 489 280.5…  →  94 489 280
  actual f  = phase_inc × f_s / 2³² = 439.999 997 6 Hz
  ```
  Frequency resolution is *f_s* / 2³² ≈ **0.000 005 Hz**. Compare that with lab 2's 444.4 Hz. This technique is called **DDS** (direct digital synthesis), and it's in every function generator and synthesiser.
  - `440 × 2³²` is about 1.9 × 10¹², far beyond `uint32_t`, so the multiplication needs **64-bit** arithmetic (`uint64_t`). The *result* fits in 32 bits again.

- **`volatile`.** `main()` writes `phase_inc` (in part B's sweep) while the interrupt reads it. Without `volatile`, the optimiser may assume nothing else changes the variable and keep a stale copy in a CPU register. `volatile` means "always really read and write memory". Any variable shared between an interrupt and the main code needs it. (Lab 4 goes deeper.)

- **Nyquist and aliasing.** With *f_s* = 20 kHz you get 20 000 snapshots per second. A tone above *f_s / 2* = **10 kHz** (the **Nyquist frequency**) is sampled less than twice per cycle, and the samples trace out a *different, lower* tone: an **alias**, at *f_s − f*. So 12 kHz sounds like 8 kHz, and 18 kHz like 2 kHz. In the part B sweep, the pitch rises to 10 kHz, then *falls* again even though the number keeps going up. In the tuning-word maths, going round "almost a full circle" per sample looks the same as going *backwards* a little.

- **Images: why the filter matters.** Each sample is *held* for 50 µs (a staircase, a "zero-order hold"). A staircase at *f* also contains copies of the tone at *f_s ± f*, *2f_s ± f*… A real DAC has a sharp filter at *f_s / 2* to remove these. Your 1.6 kHz RC filter only rolls off gently, so at high frequencies you may hear the image faintly *together with* the real tone. This is why the PCM5102A (lab 7) oversamples 8× internally.

### Parts
Nothing new: the lab 2 circuit stays as it is (R1 1 kΩ, C1 100 nF, 47 µF, R2 1 kΩ, jack). The GP26 ADC wire can stay connected. This lab doesn't use it.

### Tasks

**3.1 Build the untouched skeleton.** Pick `03-sine-dds/lab03_sine_dds` when you click Run. Expect a `'phase' defined but not used` warning until TODO 3.

**3.2 Part A: steady sine** (`PART_B 0`)
- TODO 1: build the table. TODO 5: print entries 0, 64, 128, 192 and compare with the table in Background.
- TODO 2 and 6: the tuning word and the actual frequency. Do you get 94 489 280 and 439 999 mHz?
- TODO 3 and 4: the callback and the PWM setup. Listen.
- Compare, at the same pitch: lab 1's square, lab 2's triangle, lab 3's sine. Describe the difference in Notes.
- Tuner app: how close to 440.0 Hz?

**3.3 Part B: sweep** (`PART_B 1`), TODO 7
- **Earbuds out of your ears for this one.** High frequencies are piercing.
- Listen for the pitch turning round at 10 kHz. Note where you lose hearing it (18 kHz? 15 kHz?) and where the alias comes back as a *low* tone.
- Pick three frequencies above 10 kHz and check with the tuner app that you hear *f_s − f*.

### Hints (read only if stuck)
<details><summary>TODO 1: the table</summary>

```c
float angle = 2.0f * (float)M_PI * (float)n / TABLE_SIZE;
sine_table[n] = (uint16_t)lroundf(PWM_LEVELS / 2 + 127.0f * sinf(angle));
```
- `M_PI` comes from `<math.h>`. `lroundf` rounds to the nearest integer, while a plain cast would truncate (so 254.9 → 254).
- Why 127 and not 128? 128 + 128 = 256 would be "always on", and 128 − 128 = 0 "always off". That's legal in PWM, but it makes the top and bottom of the wave asymmetric by one step. Try it and think about it.
- The `f` suffix (`2.0f`) keeps the maths in `float`. Without it, `2.0` is a `double`, which is even slower on the M0+.
</details>

<details><summary>TODO 2: tuning word</summary>

```c
return (uint32_t)(((uint64_t)hz << 32) / SAMPLE_RATE_HZ);
```
`<< 32` is × 2³². The cast to `uint64_t` must happen **before** the shift. Shifting a 32-bit value by 32 is *undefined behaviour* in C, not zero.
</details>

<details><summary>TODO 3: the callback (3 lines)</summary>

`phase += phase_inc;`, then `phase >> (32 - TABLE_BITS)` gives the index, then `pwm_set_gpio_level(AUDIO_PIN, sine_table[index]);`. No `if`, no wrap check: the overflow *is* the wrap.
</details>

<details><summary>TODO 6: actual frequency in milli-hertz</summary>

`(uint32_t)(((uint64_t)phase_inc * SAMPLE_RATE_HZ * 1000) >> 32)`. Check that the 64-bit intermediate can't overflow: 94 489 280 × 20 000 × 1000 ≈ 1.9 × 10¹⁵, and `uint64_t` goes up to 1.8 × 10¹⁹.
</details>

<details><summary>TODO 7: the sweep</summary>

```c
for (uint32_t hz = 100; hz <= 19000; hz += 100) {
    phase_inc = phase_inc_for(hz);
    if (hz % 1000 == 0) printf("%lu Hz\n", hz);
    sleep_ms(50);
}
```
</details>

### Stretch challenges
- [ ] **Melody with sines:** bring back lab 1's `struct note` array and play it by changing `phase_inc`. Notice how *easy* changing pitch is now: one assignment, no timer restart. Handle `REST` by setting `phase_inc` to 0. What does the output do then?
- [ ] **Table size:** try `TABLE_BITS` 4 (16 entries) and 10 (1024). Can you hear the difference? The coarse table makes a staircase, which adds harmonics. Why doesn't the pitch change?
- [ ] **Chord:** add a second phase accumulator and play 440 Hz + 554 Hz (a major third) together. Adding two ±127 sines gives ±254. That's out of range! Scale each by ½. What do you hear if you *don't*? *(Clipping.)*
- [ ] **Quarter-wave table:** a sine's four quarters are mirror images. Store only 64 entries and reconstruct the rest with symmetry. This saves 75 % of the memory, and real synth chips do it.
- [ ] **`const` table in flash:** generate the 256 values with a small Python script, paste them into a `static const uint16_t sine_table[] = { … };`, and drop `build_sine_table()`. Where does the table live now: RAM or flash? *(Hint: `arm-none-eabi-size build/03-sine-dds/lab03_sine_dds.elf` before and after.)*
- [ ] **Raise f_s to 40 kHz** (a 25 µs timer). Does the alias point move? What happens to the budget of CPU cycles per sample?

### Checks
- [ ] Table entries 0 / 64 / 128 / 192 print as 128 / 255 / 128 / 1
- [ ] phase_inc = 94 489 280, actual frequency ≈ 439 999 mHz
- [ ] Hear a clean sine; tuner shows ≈ 440.0 Hz
- [ ] Describe square vs triangle vs sine by ear
- [ ] Hear the sweep turn round at ~10 kHz; confirm one alias with the tuner app
- [ ] Explain why `phase` needs no wrap-around check
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:* 2026-10-01

*Table entries printed:*

*phase_inc / actual frequency printed:*
440 / 439999

*Tuner reading (part A):*
440

*Square vs triangle vs sine, how they sounded:*

*Highest frequency I could hear in the sweep:*
5000

*Aliases checked (played → heard):*

| Played | Expected alias *f_s − f* | Tuner heard |
|---|---|---|
| | | |
| | | |
| | | |

*Questions for next time:*

---

## Lab 4 — Interrupt → DMA sample feeding

### Goal
Measure what lab 3's "one interrupt per sample" really costs. Then let the **DMA** engine feed the PWM by itself, with a hardware timer setting the pace. The CPU only refills a buffer every 12.8 ms, and the audio stays clean even while the CPU is blocked.

### Background
- **What an interrupt per sample costs.** In lab 3, the timer interrupted the CPU 20 000 times a second. Each time, the CPU has to stop what it's doing, save its registers, run the SDK's timer code and your callback, schedule the next alarm, and restore everything. That's only a few microseconds, but you have just 50 µs per sample. Part A measures it: expect about **5 %** of the CPU at 20 kHz. That sounds small, but it grows with the sample rate and the number of channels. Real audio is 48 kHz stereo, and later the USB stack also needs the CPU.

- **Jitter: the bigger problem.** An interrupt can arrive *late*: while another interrupt is running, or while code has turned interrupts off (flash writes and some drivers do this). When a sample reaches the DAC late, the shape of the wave is wrong, and you hear that as noise or distortion. `DISTURB 1` imitates this: every 10 ms it switches off all interrupts for 300 µs. In Part A, six samples in a row are late. You'll hear that as a buzz at 100 Hz.

- **DMA (direct memory access)** is a copy engine next to the CPU, with its own access to memory. You give a DMA **channel** four things, and it copies on its own while the CPU does something else:

  ```text
  read address   → where to copy from     (our sample buffer; moves forward after each copy)
  write address  → where to copy to       (the PWM level register; stays the same)
  transfer count → how many copies        (BUF_LEN)
  data size      → bytes per copy         (16 bits = one uint16_t sample)
  ```
  The RP2040 has 12 channels. This lab uses two.

- **Pacing: DREQ and the DMA timer.** If nothing holds it back, DMA copies the whole buffer in a few microseconds, which is far too fast for audio. A **DREQ** ("data request") is a signal that tells a channel "copy one item now". The RP2040 has four **DMA pacing timers**. Each one sends DREQ ticks at a fraction **X / Y** of the system clock:

  ```text
  clk_sys × X / Y = 125 000 000 × 1 / 6250 = 20 000 ticks per second   (exactly)
  ```
  This ticking comes from the clock hardware, not from software. That means it never has jitter, whatever the CPU is doing.

- **Where the samples go: the PWM `CC` register.** `pwm_set_gpio_level()` from lab 2 writes a value into the slice's **CC** ("counter compare") register. The DMA now writes into that same register directly, at its address `&pwm_hw->slice[slice_num].cc`. CC holds both channels of the slice: **A in bits 0–15**, **B in bits 16–31**. A 16-bit write to it is copied into both halves, so GP15 (channel B) gets the same value. That doesn't matter here, because GP15 isn't connected. Stereo will later use 32-bit writes. The PWM only picks up a new CC value at the end of its current period, so a sample never changes halfway through a PWM pulse.

- **Double buffering ("ping-pong").** One buffer would have a problem: the CPU can't refill it while the DMA is reading it. So there are two buffers, and each one has its own channel. **Chaining** makes each channel start the other one in hardware the moment it finishes, so there's no gap between buffers:

  ```text
  time →        0 ms          12.8 ms        25.6 ms        38.4 ms
  DMA plays:    [  buffer 0  ][  buffer 1  ][  buffer 0  ][  buffer 1  ] …
  interrupt:                 ↑ "0 done"    ↑ "1 done"    ↑ "0 done"
  CPU refills:               [buf 0]        [buf 1]       [buf 0]
                              ↑ CPU has a whole buffer's time (12.8 ms) to do this
  ```
  The interrupt now happens 78 times a second instead of 20 000. In it you do two quick jobs: point the finished channel back to the start of its buffer, and tell `main()` which buffer is free.

- **Underruns, and how big a buffer should be.** If the CPU is blocked for **longer than one buffer**, the chained channel starts before anyone has rewound it or refilled its buffer. It then plays old samples, or whatever memory comes after the buffer. That's an **underrun**, and you hear it as a click. Bigger buffers allow longer blocks, but they add **latency**: a change you make (a new pitch, or later new USB audio) only reaches the speaker after the buffered audio has played. These numbers were measured on the lab board with `DISTURB_US 300`:

  | `BUF_LEN` | Audio per buffer | Underruns/s |
  |---|---|---|
  | 4 | 200 µs | ≈ 100 (every disturbance) |
  | 8 | 400 µs | 0 |
  | 256 | 12.8 ms | 0 |

  Every audio system makes this trade-off, from USB audio to your phone. Lab 8 does it again with 1 ms USB packets.

- **Measuring CPU load without a scope.** First count how many times a simple loop runs in one second while no audio is playing (`idle_loops`). Then count again with audio running. Whatever the interrupts took is missing from the count:

  ```text
  load % = 100 − loops × 100 / idle_loops
  ```

### C toolbox
New C in this lab, plus some from labs 1–3 that was never explained. Each item says what the code does, then why we need it here.

- **Fixed-width integers.** `uint8_t`, `uint16_t`, `uint32_t` and `uint64_t` (from `<stdint.h>`, which `pico/stdlib.h` includes) are **u**nsigned whole numbers of exactly 8, 16, 32 or 64 bits. `int32_t` is the signed version, so it can be negative. A plain `int` has whatever size the compiler chooses. In hardware work you want sizes you can rely on: a PWM level is 16 bits, and a phase is exactly 32 bits so that it wraps round at 2³².

- **Literal suffixes.** In `1u`, the `u` makes the 1 unsigned. In `2.0f`, the `f` makes the number a `float` instead of a `double`, which is slower on the M0+.

- **Shifts `<<` and `>>`.** These move the bits of a number to the left or right:

  ```text
  1u << 8      0000 0000 0001  →  0001 0000 0000   = 256 = 2⁸      (TABLE_SIZE)
  x  << 32     multiplies by 2³²                                    (the tuning word)
  ```
  Shifting **right** by *n* throws away the lowest *n* bits. That's how `next_sample()` gets the table index out of the phase:

  ```text
  phase (32 bits):  [ 8-bit table index | 24-bit fractional position ]
                       ↑
                   phase >> 24  →  only the top 8 bits are left: a number from 0 to 255
  ```
  You can only shift by less than the width of the type. Shifting a 32-bit value by 32 is *undefined behaviour*, which is why `phase_inc_for()` first casts to `uint64_t`.

- **Casts.** A cast like `(uint64_t)hz` or `(int32_t)x` converts a value to another type. Cast **before** the maths if the maths needs more room: `(uint64_t)loops * 100` multiplies in 64 bits. Cast **after** if you only want to change the type of the result.

- **Integer division truncates.** It drops the fractional part, so `7 / 2` is 3. `clock_get_hz(clk_sys) / SAMPLE_RATE_HZ` divides exactly at 20 kHz (6250), but not at 48 kHz (see Stretch).

- **`printf` format specifiers.** `%lu` prints an unsigned long, `%ld` a signed long, `%u` an unsigned int, and `%%` a literal `%`. On the RP2040's compiler, `uint32_t` *is* `unsigned long`, so `%lu` is correct. VS Code's checker sometimes complains about this because it thinks it's compiling for your Mac. The real build is what counts. The fully portable spelling is `"%" PRIu32` from `<inttypes.h>`.

- **2-D arrays.** `uint16_t buffers[2][BUF_LEN]` is two rows of `BUF_LEN` samples, stored one after the other in memory. `buffers[b]` is row *b*, and `buffers[b][i]` is one sample in it.

- **Pointers.** A pointer holds an **address**, which tells you where something lives in memory.
  - `uint16_t *buf` means "`buf` is the address of a `uint16_t`".
  - `buf[i]` is the *i*-th `uint16_t` from that address, so `fill_buffer(buffers[1])` writes into row 1. When you use an array's name, C passes the address of its first element.
  - `&x` means "the address of x". `&c` gives a `channel_config_set_…()` function the address of your config, so it can change *your* copy rather than a copy of its own.
  - `&pwm_hw->slice[slice_num].cc` is the address of a hardware register. On the RP2040, peripherals look like memory at fixed addresses. That's how DMA can "copy" into the PWM.
  - `a->b` means "the field `b` of the struct that pointer `a` points to". `pwm_hw` is a pointer to the PWM block's registers.

- **Structs.** A struct groups related values under one name. Lab 1's `struct note` held a pitch and a duration. `dma_channel_config` is an SDK struct holding all of a channel's settings. You change it with the setter functions, then hand it over in one go with `dma_channel_configure()`.

- **`static`** has two meanings, depending on where it's written:
  - **Outside a function** (`static uint16_t buffers…`), it makes the name private to this file.
  - **Inside a function** (`static absolute_time_t next;` in `disturb()`), the variable keeps its value between calls instead of starting fresh each time. That's how `disturb()` remembers when it last ran.

- **`volatile`, in more depth (from lab 3).** `buffer_to_fill` is written by the interrupt and read by `main()`. `volatile` makes every read really go to memory. But it doesn't make a multi-step operation safe. If you write `if (buffer_to_fill != -1) fill_buffer(buffers[buffer_to_fill]);`, that reads the variable **twice**, and the interrupt could change it in between. So `service_audio()` copies it into a local variable once and uses that copy.

- **Callbacks: passing a function to a function.** `irq_set_exclusive_handler(DMA_IRQ_0, dma_handler)` passes `dma_handler` *without* `()`. That hands over the function itself, so the hardware can call it later, rather than calling it now. Lab 3's `add_repeating_timer_us(…, sample_callback, …)` did the same thing. The parameter that receives it is a **function pointer**.

- **Bit masks and "write 1 to clear".** A register often packs one flag per bit. The DMA's interrupt-status register has bit *n* for channel *n*. `1u << n` builds a **mask** with only bit *n* set. The SDK's `dma_channel_acknowledge_irq0(chan)` writes `1u << chan` to that register. In this register, writing a 1 *clears* the flag and writing 0 changes nothing, so you can clear one channel's flag without touching the others.

- **`#if` vs `if`.** `#if PART_B` is handled by the preprocessor *before* compiling: the code that's switched off is removed completely, as if you'd never typed it. `if (DISTURB)` is ordinary C. The compiler still checks the code, and because `DISTURB` is a constant 0 or 1, the optimiser removes the dead branch anyway. Use `#if` when the code in the other branch wouldn't even compile, or shouldn't exist at all.

### Parts
Nothing new: the lab 2 circuit stays as it is.

### Tasks

**4.1 Build the untouched skeleton.** Pick `04-dma-feed/lab04_dma_feed` when you click Run. Expect two `defined but not used` warnings (`service_audio`, `disturb`) until TODO 1.

**4.2 Part A: what the timer interrupt costs** (`PART_B 0`)
- TODO 1: the counting loop. TODO 2: work out and print the load.
- Run it and write down `Idle` loops, loops with audio, and the load.
- Set `SAMPLE_RATE_HZ` to 40000, then 48000, and write down the load each time. The pitch stays at 440 Hz. Why? Set it back to 20000.
- `DISTURB 1`: listen. What does the 300 µs gap every 10 ms do to the sine?

**4.3 Part B: DMA** (`PART_B 1`, `DISTURB 0`)
- TODO 3: `fill_buffer()`. TODO 4: the pacing timer. TODO 5: the two channels. TODO 6: the interrupt handler. TODO 7: `service_audio()`.
- You should hear the same sine as before. Check that the timer line prints `1/6250 of 125000000 Hz`, and note the load.
- `DISTURB 1`: listen again. Compare with Part A. Check that underruns stay at 0.

**4.4 Break it on purpose.** With `DISTURB 1`, set `BUF_LEN` to 4. Watch the underrun count and listen. Then find the smallest `BUF_LEN` with 0 underruns, and explain it using `DISTURB_US` and the sample period. Set it back to 256.

### Hints (read only if stuck)
<details><summary>TODO 1: the counting loop</summary>

```c
while (!time_reached(end)) {
    service_audio();
    if (DISTURB) {
        disturb();
    }
    loops++;
}
```
`time_reached(end)` turns true once the clock passes `end`, so `!` ("not") keeps the loop going until then.
</details>

<details><summary>TODO 2: load</summary>

```c
int32_t load = 100 - (int32_t)((uint64_t)loops * 100 / idle_loops);
printf("loops/s %lu  load %ld %%  underruns %lu\n", loops, load, underruns);
```
- `loops` is about 6 million, and 6 million × 100 ≈ 0.6 billion, which fits under `uint32_t`'s 4.29 billion. A faster loop or a longer measuring window would overflow, so the 64-bit cast is a cheap safety margin.
- The load is `int32_t` because measurement noise can make `loops` a little *bigger* than `idle_loops`, giving a slightly negative number. An unsigned type would turn −1 into 4 294 967 295.
</details>

<details><summary>TODO 3: fill_buffer</summary>

```c
for (uint32_t i = 0; i < BUF_LEN; i++) {
    buf[i] = next_sample();
}
```
</details>

<details><summary>TODO 4: pacing timer</summary>

```c
int timer = dma_claim_unused_timer(true);
uint32_t denom = clock_get_hz(clk_sys) / SAMPLE_RATE_HZ;
dma_timer_set_fraction(timer, 1, denom);
printf("DMA timer: 1/%lu of %lu Hz\n", denom, clock_get_hz(clk_sys));
```
`true` means "I need one; stop with an error if none is free". Working the fraction out from `clock_get_hz()`, rather than writing 6250, keeps it correct if the clock ever changes.
</details>

<details><summary>TODO 5: the two channels</summary>

```c
channel_config_set_dreq(&c, dma_get_timer_dreq(timer));
channel_config_set_chain_to(&c, dma_chan[1 - b]);
dma_channel_configure(dma_chan[b], &c,
                      &pwm_hw->slice[slice_num].cc,     // write here
                      buffers[b],                       // read from here
                      BUF_LEN, false);                  // this many, don't start yet
dma_channel_set_irq0_enabled(dma_chan[b], true);
```
`1 - b` is the *other* channel: 1 when b is 0, and 0 when b is 1.
</details>

<details><summary>TODO 6: the interrupt handler</summary>

```c
dma_channel_acknowledge_irq0(dma_chan[b]);
dma_channel_set_read_addr(dma_chan[b], buffers[b], false);
if (buffer_to_fill != -1) {
    underruns++;
}
buffer_to_fill = b;
```
- `false` means "set the address but don't start". The *other* channel's chaining starts this one later.
- The transfer count needs no reset: each time a channel starts, it reloads the count you configured.
- If you forget to acknowledge, the interrupt stays active and the handler runs again and again forever, so `main()` never runs.
</details>

<details><summary>TODO 7: service_audio</summary>

```c
int b = buffer_to_fill;     // read the volatile variable exactly once
if (b != -1) {
    fill_buffer(buffers[b]);
    buffer_to_fill = -1;
}
```
</details>

### Stretch challenges
- [ ] **Exact 48 kHz:** at 48 kHz, `125 000 000 / 48 000` = 2604.17, so the integer division gives a slightly wrong rate. Which rate exactly? Find X / Y with both under 65 536 so that 125 000 000 × X / Y = 48 000 exactly. *(Hint: reduce 48 000 / 125 000 000 as a fraction.)*
- [ ] **Hear the latency:** play lab 3's melody in Part B by changing `phase_inc` from `main()`. With `BUF_LEN` 4096 (about 0.2 s per buffer), can you hear the notes start late? Why can the delay be up to *two* buffers long?
- [ ] **Stereo preview:** make the buffers `uint32_t`, use `DMA_SIZE_32`, and store `left | ((uint32_t)right << 16)`. Play 440 Hz on A (GP14) and 660 Hz on B (GP15). You'll need GP15 set up for PWM, and a second filter if you want to listen to it.
- [ ] **Refill inside the interrupt:** call `fill_buffer()` from `dma_handler()` instead of `main()`. What's good about it? What happens to the other interrupts while it runs?

### Checks
- [x] Part A load printed: about 5 % at 20 kHz; also written down at 40 kHz and 48 kHz
- [ ] Part A with `DISTURB 1`: heard the disturbance
- [ ] Part B plays the same clean sine; timer prints `1/6250 of 125000000 Hz`
- [ ] Part B load about 1 %
- [ ] Part B with `DISTURB 1` and `BUF_LEN 256`: clean sound, 0 underruns
- [ ] Found the smallest `BUF_LEN` with 0 underruns, and explained why
- [ ] Explain why `buffer_to_fill` is `volatile` *and* why `service_audio()` reads it only once
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:*

*Part A (timer):* idle loops / loops with audio / load %:

| Sample rate | Load % |
|---|---|
| 20 kHz |5% |
| 40 kHz |10% |
| 48 kHz |13% |

*Part A with DISTURB, how it sounded:*
interrupted

*Part B (DMA):* timer line printed / load %:

*Part B with DISTURB, how it sounded / underruns:*

*Smallest BUF_LEN with 0 underruns, and why:*

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
