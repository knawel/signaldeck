# signaldeck Labbook, part 2: PIO, I2S and USB audio

Labs 0–5 in [part 1](labbook.md) built three DACs by hand: PWM, a sine synth fed by interrupts and DMA, and an R-2R ladder. Part 2 moves to the tools the real device uses: **PIO** state machines (lab 6), the **PCM5102A** over I2S (lab 7), and finally **USB audio** (lab 8).

**How to use this book:** the same rules as part 1. Each lab has **Goal → Background → Wiring → Tasks → Checks → Notes**. Skeletons live in `firmware/labs/NN-name/`; replace the `TODO`s yourself and use the hints only when stuck. A lab is done when every box in its **Checks** list is ticked. The board layout and pin plan are in part 1, under [Prototype board: base layout](labbook.md#prototype-board-base-layout).

"Lab N" with N ≤ 5 means a lab in [part 1](labbook.md). "C1–C3" are the [C side-labs](c-labbook.md). C3 (register structs and `volatile`) is worth finishing before lab 6.

## Lab index

| # | Lab | Status |
|---|---|---|
| 0–5 | Toolchain → R-2R ladder: see [part 1](labbook.md) | ✅ done |
| 6 | Sigma-delta DAC through PIO | ⬜ next |
| 7 | PCM5102A over I2S | ⬜ |
| 8 | USB audio → PCM5102A (main firmware) | ⬜ |

---

## Lab 6 — Sigma-delta DAC through PIO

### Goal
Go back to **one pin and lab 2's RC filter**, but drive it in a smarter way. Instead of PWM's one pulse per period, send a **1-bit stream at 1.28 Mbit/s** whose 1s are spread out as evenly as possible. The same filter then gives you 65 536 DC levels instead of 256, with less ripple. The bits are shifted out by a **PIO state machine**, a tiny programmable processor that drives pins with clock-exact timing, and the CPU only computes them.

### Background
- **Where this fits.** Lab 2 made levels from *time* (PWM). Lab 5 made them from *resistor ratios*, and you measured how hard it is to match resistors well. A **sigma-delta** (ΣΔ) DAC goes back to timing, because timing is something a chip can do precisely. The PCM5102A in lab 7 is a sigma-delta DAC, and so is nearly every audio DAC made today.

- **Same average, different arrangement.** A PWM pin and a ΣΔ pin can be high for exactly the same fraction of the time. The difference is *how the 1s are arranged*. Take 25 % (8 of 32 bits):

  ```text
  PWM-like:  11111111 00000000 00000000 00000000     one pulse, repeats every 32 bits
  ΣΔ:        1000 1000 1000 1000 1000 1000 1000 1000  spread out, repeats every 4 bits
  ```
  Both average to 0.25 × 3.3 V. But the ΣΔ pattern repeats **8 times faster**, so the leftover ripple is at a frequency 8 times higher. Lab 2's filter removes it far better there. Part A measures exactly this, with the patterns `0x000000FF` and `0x11111111`.

- **The modulator: lab 3's phase accumulator, again.** How do you spread the 1s evenly for *any* level? Use an accumulator and output its **carry**:

  ```text
  each bit:   sum = acc + x          (x = the level, 0 … 65535)
              bit = 1 if sum ≥ 65536 (the carry out of 16 bits), else 0
              acc = sum − 65536·bit  (keep the low 16 bits)
  ```
  It's exactly lab 3's clock face. `x` is how far you step per bit, and every time you go past 12 o'clock you output a 1. A step of x / 65536 of a turn per bit gives x / 65536 ones per bit, so the average is **x / 65536 × 3.3 V**.

  Small example with a 4-bit accumulator (0 … 15) and x = 5, so 5/16 of the bits should be 1:

  ```text
  bit #:   1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
  sum:     5 10 15 20  9 14 19  8 13 18  7 12 17  6 11 16
  bit:     0  0  0  1  0  0  1  0  0  1  0  0  1  0  0  1    → 5 ones in 16 bits, evenly spaced
  acc:     5 10 15  4  9 14  3  8 13  2  7 12  1  6 11  0
  ```

- **Why it works so well: the error is carried, not thrown away.** A 1-bit output is always wrong: it's 0 or 3.3 V, never the 1.0 V you want. What `acc` holds is **the error so far**. Each bit is chosen to cancel the error the earlier bits left behind. So over *any* window of N bits, the number of 1s is within one of the exact x·N / 65536.
  - Average over a **short** window (high frequencies), and one bit of error is a lot.
  - Average over a **long** window (low frequencies, i.e. audio), and the same one bit is tiny.

  So the quantization noise isn't gone. It has been **pushed up to high frequencies**, where the filter removes it. This is called **noise shaping**, and it's the "delta" (difference) and "sigma" (sum) in the name: `acc` sums the differences between what you want and what you sent.

- **What you gain over PWM.** With the same pin, filter and 125 MHz clock:

  | | 8-bit PWM (lab 2) | 1st-order ΣΔ (this lab) |
  |---|---|---|
  | DC levels | 256 | 65 536 |
  | Ripple at 50 % | ≈ 17 mV at 488 kHz | ≈ 13 mV at 640 kHz |
  | Audio resolution, 0–10 kHz (theory) | ≈ 8 bits, 50 dB | ≈ 9 bits, 57 dB |
  | Weak spot | coarse steps | **idle tones** (task 6.5) |

  For a *steady* level, ΣΔ is far better. For audio, a 1st-order modulator with 64 bits per sample is only a little better than 8-bit PWM. Every doubling of the bit rate adds about 9 dB (1.5 bits), and a **2nd-order** modulator (stretch) adds 15 dB per doubling: about 85 dB, or 14 bits, at the same 1.28 Mbit/s. The PCM5102A uses a higher-order modulator, a much higher rate and a filter on the chip.

- **Oversampling.** Each audio sample becomes `OVERSAMPLE` = 64 bits, so the bit rate is 20 000 × 64 = **1.28 Mbit/s**. The PIO takes 32 bits per FIFO word, so that's **2 words per sample**.

- **PIO: Programmable I/O.** The RP2040 has two PIO blocks, `pio0` and `pio1`. Each has **4 state machines** and room for **32 instructions** shared between them. A state machine is a tiny CPU built only for moving bits between pins and FIFOs:

  ```text
          CPU / DMA                           state machine                         pins
    ┌───────────────┐   TX FIFO   ┌──────────────────────────────────────┐
    │ pio_sm_put()  │ ──[][][][]─→│ OSR (output shift register, 32 bits) │ ── out pins ──→ GP14
    └───────────────┘   4 words   │ ISR, X, Y, PC, clock divider          │
                       (8 joined) └──────────────────────────────────────┘
  ```
  - It has only **9 instructions** (`jmp wait in out push pull mov irq set`), and **each one takes exactly one cycle**. That's what makes PIO timing exact: there's no cache, no interrupt and no other code to delay it.
  - `out pins, 1` shifts **1 bit** out of the OSR onto the pins. It's the only instruction this lab needs.
  - **Autopull:** once 32 bits have been shifted out, the OSR refills itself from the TX FIFO without spending an instruction. If the FIFO is empty, the state machine **stalls** on the `out` and the pin keeps its last value. The block's `FDEBUG` register then sets a `TXSTALL` flag for that state machine. It's **write 1 to clear**, like lab 4's DMA interrupt flags. `sd_stalled()` reads and clears it.
  - **`.wrap_target` / `.wrap`** make the program loop back for free. One instruction between them means "`out pins, 1` forever, one per cycle".
  - **Name clash:** the PIO datasheet's **OSR** is the *output shift register*. In audio, "OSR" usually means *oversampling ratio*. This lab calls the ratio `OVERSAMPLE` to keep them apart.

- **The clock divider sets the bit rate.** One instruction per cycle, and one bit per instruction, so the bit rate is the state machine's clock: **clk_sys / divider**. The divider is a **16.8 fixed-point** number: 16 integer bits and 8 fractional bits (1/256ths).

  ```text
  divider = 125 000 000 / 1 280 000 = 97.65625 = 97 + 168/256    ← exactly representable
  ```
  A fractional divider can't make every cycle 97.656 clocks long. It makes some 97 and some 98 so the **average** is exact. Each bit edge moves by up to one 8 ns system clock. That's **jitter**, but it's small next to a 781 ns bit.

- **The PIO paces the CPU.** In Part C there's no timer and no interrupt. The main loop computes words and hands them over with `pio_sm_put_blocking()`, which **waits while the FIFO is full**. The state machine empties it at exactly 1.28 Mbit/s, so it sets the pace for everything. This is how real audio hardware works: the DAC's clock is the master, and software just has to keep up. It's also why lab 5 pointed here: DMA can't reach the SIO pins, but it *can* feed a PIO FIFO (see Stretch).

- **CPU budget.** 125 MHz / 1.28 Mbit/s ≈ 98 CPU cycles per bit. The modulator needs roughly 10 cycles per bit, so expect ~10 % of the CPU in Part C.

- **Idle tones: the 1st-order weak spot.** With a level of exactly 32 768 (½), the stream is `1010…`: a 640 kHz square wave that the filter removes completely. Silence. Now set 32 768 + 64. The pattern is still mostly `1010…`, but the accumulator creeps forward by 64 every bit, and each time the creep adds up to half a turn the pattern **slips** by one bit. The slips repeat at

  ```text
  f_idle = BIT_RATE × offset / 32768 = 1 280 000 × 64 / 32768 = 2500 Hz
  ```
  That's a quiet **whistle** in the middle of what should be silence. With a moving signal like a sine the pattern never settles, so you don't hear it there. But real audio has plenty of near-silence. Higher-order modulators and **dither** (adding a little noise on purpose, see Stretch) break these patterns up.

### C toolbox
New C in this lab. Shifts, masks, `(void)`, `static`, `volatile` and casts are in the lab 4 and lab 5 toolboxes.

- **Catching a carry with a wider type.** `sd_acc` is a `uint16_t`, and `x` goes up to 65 535. Their sum needs **17 bits**, so add them in 32 bits:

  ```c
  uint32_t sum = (uint32_t)sd_acc + x;    // 0 … 131 070: never overflows
  uint32_t bit = sum >> 16;               // bit 16 = the carry: 0 or 1
  sd_acc = (uint16_t)sum;                 // the cast keeps only the low 16 bits (same as sum & 0xFFFF)
  ```
  Casting to a smaller unsigned type **throws away the high bits**. That's the same wrap-around as lab 3's phase, used on purpose again.

- **Building a word bit by bit with `|=`.** `a |= b` is short for `a = a | b`. `word |= bit << n` moves the bit to position `n` and ORs it in, leaving the other bits alone. Start with `word = 0` and do this for n = 0 … 31. Bit 0 goes out first, because the state machine shifts **right**.

- **`#if` / `#elif` / `#else`.** Lab 4's `#if` with more branches. `#if PART == 0 … #elif PART == 1 … #else … #endif` keeps exactly one branch. The others disappear before compiling, which is why each part only gets warnings about its *own* unfinished TODOs.

- **Counting an array: `sizeof a / sizeof a[0]`.** `sizeof patterns` is the size of the whole array in bytes (6 × 4 = 24), and `sizeof patterns[0]` is one element (4), so the ratio is the number of elements. Add a pattern and the loop picks it up automatically. It only works on a real array: once an array has been passed to a function, it's just a pointer, and `sizeof` gives the pointer's size.

- **`static const` inside a function.** `static const uint32_t patterns[] = {…}` is built once by the compiler, kept in flash, and never copied onto the stack. You've seen both words before (lab 4's `static` inside a function, lab 3's `const` stretch). Together they're the normal way to write a fixed table.

- **Returning a struct.** `measure()` returns a `struct reading` with four fields: the mean, min, max and a stall flag. A function can only return one thing, and a struct makes that one thing a bundle. It's copied back to the caller, which is fine for a few bytes. `struct reading r = { 0.0f, 4095, 0, false };` sets the fields in the order they're declared.

- **Characters are small numbers.** `'A'` is 65 in ASCII, so `'A' + PART` is `'A'`, `'B'` or `'C'`. `%c` prints a number as its character.

- **A header made during the build.** `#include "sigma_delta.pio.h"` doesn't exist in the source folder. CMake's `pico_generate_pio_header()` runs **pioasm**, the PIO assembler, on `sigma_delta.pio` and writes the header into `build/06-sigma-delta/`. Open it after building: your one instruction is the number `0x6001`, and `sigma_delta_program_get_default_config()` is written for you there.

- **`SD_PIO->fdebug`.** `pio0` is a pointer to a `pio_hw_t` struct laid over the PIO block's registers, the same idea as `sio_hw` in C3. `PIO_FDEBUG_TXSTALL_LSB` (24) is the bit where the TXSTALL flags start: bit 24 + sm is the flag for state machine `sm`.

### Parts
Nothing new: lab 2's circuit on GP14 (R1 1 kΩ, C1 100 nF, 47 µF, R2 1 kΩ, jack) and the GP26 ADC wire. The R-2R ladder can stay on the board. This lab doesn't drive GP0–GP7, so their pull-downs hold the ladder at 0 V.

### Wiring (zone B)
Lab 2's circuit, unchanged:

```text
            R1 1 kΩ      FILT                    47 µF        R2 1 kΩ
  GP14 ───[ 1 kΩ ]───────┬──────────┬───────(+)┤├(−)────────[ 1 kΩ ]──┬── jack TIP
                         │          │                                  └── jack RING
                    C1 ═══ 100 nF   └────── GP26 (ADC0)
                         │
  GND rail ──────────────┴───────────────────────────────────────────────── jack SLEEVE
```

- **Move the 47 µF's (+) leg back from the ladder's OUT to FILT.** You moved it in task 5.3.
- **Earbuds out while you read numbers** (Parts A and B), as in lab 5: the 47 µF charging through the earbuds makes the readings drift. Plug them in to listen.

### Tasks

**6.1 Build the untouched skeleton.** Pick `06-sigma-delta/lab06_sigma_delta` when you click Run. Part A (`PART 0`) builds with zero warnings. Part B warns that `sd_acc` is unused until TODO 4, and Part C also warns about `phase` and `sd_next_word` until TODO 6. Flash Part A as it is: every reading is about 0. Why? *(Look at the instruction in `sigma_delta.pio`.)*

**6.2 Part A: the PIO on its own** (`PART 0`, earbuds out)
- TODO 1: the PIO instruction. TODO 2: the state machine setup. Check that the divider prints **97.65625**.
- TODO 3: count the 1s in each pattern and print the expected ADC counts.
- Copy the six lines into the table in Notes. Compare the measured **ripple** (max − min) with the prediction from lab 2's formula, ripple_pp ≈ 3.3 V · D(1−D) · T / (R1·C1), where T is the time the pattern takes to repeat. 1 mV ≈ 1.24 ADC counts. Expect about 10 extra counts of ADC noise on top.

  | Pattern | 1s | Expected | Repeats every | Predicted ripple |
  |---|---|---|---|---|
  | `0x00000000` | 0 | 0 | — | 0 |
  | `0x11111111` | 8 | 1024 | 4 bits = 3.1 µs | ≈ 24 counts |
  | `0x000000FF` | 8 | 1024 | 32 bits = 25 µs | ≈ 190 counts |
  | `0x55555555` | 16 | 2048 | 2 bits = 1.6 µs | ≈ 16 counts |
  | `0x0000FFFF` | 16 | 2048 | 32 bits = 25 µs | ≈ 255 counts |
  | `0xFFFFFFFF` | 32 | 4096 (the ADC stops at 4095) | — | 0 |
- During the 5 s hold, check FILT with the multimeter: ≈ 1.65 V.
- **Question:** `0x11111111` and `0x000000FF` have the same mean. Which one is PWM-like? Why is its ripple about 8 times bigger?

**6.3 Part B: the modulator** (`PART 1`, earbuds out)
- Before coding, work the 4-bit example from Background by hand for x = 3 (3/16). Where do the 1s land?
- TODO 4: `sd_next_word()`. TODO 5: the expected ADC counts.
- The range levels (0 … 65535) should match within a few counts. Expect the same small offset and gain error as lab 5's ADC, so judge the shape, not the last count.
- **The fine levels** are 32768 + 16, + 64 and + 256. Subtract the 32768 reading from each: you should see about **+1, +4 and +16** counts. 16 counts is one of lab 2's 8-bit steps; lab 2's PWM couldn't make anything in between.
- The ripple should stay small at every level. Compare with Part A's bunched patterns.

**6.4 Part C: a sine through the modulator** (`PART 2`)
- TODO 6: the sample loop. Earbuds in.
- Listen and compare with lab 3 (PWM) and lab 5 (ladder) at the same 440 Hz. Is there any hiss? Tuner: 440 Hz?
- The program prints the stall count every 2 s. It should stay at **0**. If it goes up by one with every print, that's `printf` itself holding up the loop for longer than the FIFO lasts (8 words = 200 µs).

**6.5 Break it on purpose: idle tones** (`PART 1`)
- Earbuds in for the 5 s "listen" hold. Try `IDLE_OFFSET` 0, 16, 64 and 256.
- At 0 you should hear nothing. For the others, predict f_idle = 1 280 000 × offset / 32768 first, then check with the tuner app. 16 may be too quiet to hear; 256 lands at 10 kHz.
- **Question:** why don't you hear any whistle in Part C, even though it uses the same modulator?

### Hints (read only if stuck)
<details><summary>TODO 1: the PIO instruction</summary>

```text
    out pins, 1
```
Destination `pins`, bit count `1`. The pin it writes to isn't in the program. It comes from `sm_config_set_out_pins()` in TODO 2, so the same program works on any pin.
</details>

<details><summary>TODO 2: state machine setup</summary>

```c
sm_config_set_out_pins(&c, AUDIO_PIN, 1);
sm_config_set_out_shift(&c, true, true, 32);    // shift right, autopull, every 32 bits
float div = (float)clock_get_hz(clk_sys) / BIT_RATE_HZ;
sm_config_set_clkdiv(&c, div);
printf("PIO: %lu Hz / %.5f = %lu bits/s\n", clock_get_hz(clk_sys), div, (uint32_t)BIT_RATE_HZ);
```
The `(float)` cast comes first, so the division keeps its fractional part. Without it, `125000000 / 1280000` would be integer division: 97, and the bit rate would be 0.7 % too fast.
</details>

<details><summary>TODO 3: count the 1s</summary>

```c
for (uint32_t k = 0; k < 32; k++) {
    ones += (pattern >> k) & 1u;
}
uint32_t expected = ones * 4096 / 32;
```
Lab 5's "reading bit *k*" trick, added up. GCC also has `__builtin_popcount()`, which does it in one call.
</details>

<details><summary>TODO 4: the modulator</summary>

```c
uint32_t sum = (uint32_t)sd_acc + x;
uint32_t bit = sum >> 16;
sd_acc = (uint16_t)sum;
word |= bit << n;
```
Delete `(void)x;` afterwards.
</details>

<details><summary>TODO 5: expected counts</summary>

`uint32_t expected = level >> 4;` Shifting right by 4 divides by 2⁴ = 16, and 65536 / 16 = 4096.
</details>

<details><summary>TODO 6: the sample loop</summary>

```c
phase += phase_inc;
uint16_t x = sine_table[phase >> (32 - TABLE_BITS)];
for (uint32_t w = 0; w < WORDS_PER_SAMPLE; w++) {
    pio_sm_put_blocking(SD_PIO, sm, sd_next_word(x));
}
```
Lab 3's callback, except that the "output" is now 64 bits sent in two words, and nothing calls it: it runs as fast as `pio_sm_put_blocking()` lets it.
</details>

### Stretch challenges
- [ ] **Dither:** in Part B, add a small random number to `x` for every word (for example `x + (rand() & 63) - 32`, with `#include <stdlib.h>`). That's fine near the middle of the range; near 0 or 65535 the sum would wrap round, so you'd have to clamp it. Listen to `IDLE_OFFSET 64` again. What happened to the whistle, and what did you get instead?
- [ ] **2nd-order modulator:** two integrators instead of one. With a signed level `xs = x − 32768` and `int32_t` integrators `e1`, `e2`:

  ```text
  bit = (e2 >= 0) ? 1 : 0
  fb  = bit ? 32767 : −32768
  e1 += xs − fb
  e2 += e1 − fb
  ```
  Write it as `sd2_next_word()` and play the sine. Is the hiss lower? Are the idle tones still there? Keep the sine amplitude at about 80 % of full scale: a 2nd-order loop can become unstable near full scale.
- [ ] **Measure the CPU cost:** time 20 000 samples with `time_us_64()` in Part C. The PIO paces the loop, so it always takes 1 s. Instead, time only the `sd_next_word()` calls. What percentage of each second is that?
- [ ] **Half the bits:** set `OVERSAMPLE` to 32 (640 kbit/s). Theory says about 9 dB more noise in the audio band. Can you hear it? What happens to Part A's ripple numbers?
- [ ] **Stall on purpose:** in Part C, add `busy_wait_us(300)` once every 1000 samples. Watch the stall count and listen. This is lab 4's underrun again, from the other side.
- [ ] **DMA feeds the PIO:** bring back lab 4's two ping-pong buffers, now `uint32_t` words. The write address is `&SD_PIO->txf[sm]`, the size `DMA_SIZE_32`, and the pacing DREQ is `pio_get_dreq(SD_PIO, sm, true)`: the TX FIFO itself asks for data. No pacing timer is needed. Why not?

### Checks
- [ ] Untouched Part A reads ~0, and I know why
- [ ] PIO divider prints 97.65625
- [ ] Part A: all six means within a few counts of expected; ripple table filled in and compared with the prediction
- [ ] Explained why `0x000000FF` ripples more than `0x11111111`
- [ ] Part B: range levels match; fine levels show about +1 / +4 / +16 counts
- [ ] Part C: clean 440 Hz sine, 0 stalls
- [ ] Heard at least one idle tone and matched its pitch to f_idle = BIT_RATE × offset / 32768
- [ ] Explain why `pio_sm_put_blocking()` makes a timer unnecessary
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:*

*Divider printed:*

*Part A: patterns:*

| Pattern | Expected | Mean | Ripple (max − min) | Predicted ripple |
|---|---|---|---|---|
| `0x00000000` | 0 | | | 0 |
| `0x11111111` | 1024 | | | ≈ 24 |
| `0x000000FF` | 1024 | | | ≈ 190 |
| `0x55555555` | 2048 | | | ≈ 16 |
| `0x0000FFFF` | 2048 | | | ≈ 255 |
| `0xFFFFFFFF` | 4096 (ADC max 4095) | | | 0 |

*Multimeter at FILT during the 0x55555555 hold:*

*Part B: levels:*

| Level | Expected | Mean | Mean − mean(32768) |
|---|---|---|---|
| 0 | 0 | | |
| 4096 | 256 | | |
| 16384 | 1024 | | |
| 32768 | 2048 | | 0 |
| 49152 | 3072 | | |
| 61440 | 3840 | | |
| 65535 | 4095 | | |
| 32784 (+16) | 2049 | | ≈ 1 |
| 32832 (+64) | 2052 | | ≈ 4 |
| 33024 (+256) | 2064 | | ≈ 16 |

*Part C: how it sounded next to lab 3 (PWM) and lab 5 (ladder); stalls:*

*Idle tones (offset → predicted Hz → heard / tuner):*

*Questions for next time:*

---

## Lab 7 — PCM5102A over I2S

*To be written.* Plan: wire the PCM5102A module, write the I2S PIO program (BCK, LRCK and DATA, using side-set), feed it lab 3's sine through DMA at 48 kHz / 24 bits, and compare it with every DAC from labs 2, 5 and 6.

---

## Lab 8 — USB audio → PCM5102A (main firmware)

*To be written.* Plan: TinyUSB UAC2 playback in `firmware/rp2040`, USB packets into lab 7's I2S buffers, and the clock drift between the computer and the DAC.

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
