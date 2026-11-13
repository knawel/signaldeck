# signaldeck Labbook, part 2: PIO, I2S and USB audio

Labs 0–5 in [part 1](labbook.md) built three DACs by hand: PWM, a sine synth fed by interrupts and DMA, and an R-2R ladder. Part 2 moves to the tools the real device uses: **PIO** state machines (lab 6), the **PCM5102A** over I2S (lab 7), and finally **USB audio** (lab 8).

**How to use this book:** the same rules as part 1. Each lab has **Goal → Background → Wiring → Tasks → Checks → Notes**. Skeletons live in `firmware/labs/NN-name/`; replace the `TODO`s yourself and use the hints only when stuck. A lab is done when every box in its **Checks** list is ticked. The board layout and pin plan are in part 1, under [Prototype board: base layout](labbook.md#prototype-board-base-layout).

"Lab N" with N ≤ 5 means a lab in [part 1](labbook.md). "C1–C3" are the [C side-labs](c-labbook.md). C3 (register structs and `volatile`) is worth finishing before lab 6.

## Lab index

| # | Lab | Status |
|---|---|---|
| 0–5 | Toolchain → R-2R ladder: see [part 1](labbook.md) | ✅ done |
| 6 | Sigma-delta DAC through PIO | ✅ done |
| 7 | PCM5102A over I2S | 🔶 checks open |
| 8 | USB audio → PCM5102A | ⬜ next |

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

### Goal
Drive a real audio DAC chip. Write the **I2S** PIO program yourself, check its timing by having the RP2040 read its own pins back, then play a **48 kHz / 24-bit stereo** sine through the PCM5102A and measure its level in **dBFS**. Finally, let DMA feed the PIO, so the CPU only has to refill a buffer now and then. That's the audio half of lab 8's USB DAC.

### Background
- **Where this fits.** Labs 2, 5 and 6 each built a DAC from scratch: PWM, the ladder and sigma-delta. The PCM5102A is a whole sigma-delta DAC on one chip. It's the same idea as lab 6, but done much better: an 8× interpolation filter, a higher-order modulator running at several MHz, a filter on the chip and a proper output stage. Your job shrinks to sending it numbers, in the format it expects. That format is **I2S** (Inter-IC Sound).

- **The three I2S signals.**

  | Signal | Module pin | What it does | Here |
  |---|---|---|---|
  | **BCK** (bit clock) | BCK | One period per data bit. The DAC reads DIN on each **rising** edge. | GP0 |
  | **LRCK** (left/right clock, also "WS") | LCK | 0 = left channel, 1 = right channel. One full LRCK period = one **frame** = one sample for each ear, so it runs at the sample rate. | GP1 |
  | **DIN** (data) | DIN | The sample bits, **MSB first**. Changes while BCK is low. | GP2 |
  | SCK (system clock) | SCK | Optional. **Tie it to GND**, and the chip makes its own clock from BCK with a PLL. | GND |

  There are no addresses and no handshake. Data flows one way, and the DAC simply trusts the clocks.

- **One frame, 64 BCK periods.** Each channel gets a **32-bit slot**. The sample is 24 bits, so it sits in the top 24 bits and the bottom 8 are zero. 2 slots × 32 bits = 64 BCK periods per frame, so:

  ```text
  BCK  = 48 000 frames/s × 64 = 3.072 MHz
  ```
  The PCM5102A accepts 32, 48 or 64 BCK periods per frame. Pick anything else and it mutes.

- **The one-bit delay.** This is the detail that makes I2S I2S: **LRCK changes one BCK period before the MSB.** So the last bit (LSB) of each slot goes out while LRCK is already showing the *next* channel:

  ```text
  BCK rising edge:   1    2    3   …   32 │  33   34   …   64 │  1 (next frame)
  LRCK:              0    0    0   …    0 │   1    1   …    1 │  0
  DIN (bit read):   R0   L31  L30  …   L1 │  L0   R31  …   R1 │  R0
                    ↑ the previous right LSB      ↑ left LSB, LRCK already 1
  ```
  `L31` is bit 31 of the left slot (the sample's sign bit), and `R0` is the right slot's bit 0. Why build it this way? The DAC sees LRCK change and has one whole bit period to get ready before the MSB arrives. The other common format, **left-justified**, has no delay: LRCK and the MSB change together. The PCM5102A does both, chosen by its **FMT** pin. The draft main firmware in `firmware/rp2040/src/i2s.pio` is written for left-justified. Lab 8 replaces it with yours.

- **Side-set: changing pins for free.** Lab 6's program had one job, `out pins, 1`. Here, every BCK half-period also has to set BCK and LRCK, and spending an instruction on that would halve your speed. **Side-set** solves it: each instruction has 5 spare bits (normally a delay count), and `.side_set 2` gives 2 of them to driving 2 pins. Those pins change **in the same cycle** as the instruction, whatever the instruction is:

  ```text
  out pins, 1   side 0b00     ; DIN gets the next bit, and LRCK = 0, BCK = 0, all in one cycle
  jmp x-- loop  side 0b01     ; count and loop, and BCK = 1
  ```
  `side 0bLB`: bit 1 drives LRCK, bit 0 drives BCK. The side-set pins are always **neighbours**, starting at the base pin you give the state machine. That's why LRCK must be GP1 when BCK is GP0, and why `main.c` has a `_Static_assert` to check it.

- **Two instructions per bit.** One instruction runs with BCK low (`out`: DIN changes), the next with BCK high (the DAC reads DIN; the instruction just counts). So the state machine runs at **2 × BCK = 6.144 MHz**, and a frame is 128 instructions. The loop counter is the scratch register **X**: `set x, n` loads it, and `jmp x-- label` jumps back while X wasn't 0, counting down on each pass. Starting from n, the loop body runs **n + 1** times.

- **A clock you can't quite hit.** 125 MHz / 6.144 MHz = 20.3450…, and the divider only has 1/256 steps. The SDK rounds down to 20 + 88/256 = 20.34375, so:

  ```text
  sample rate = 125 000 000 / 20.34375 / 128 = 48 003.07 Hz    (+64 ppm)
  ```
  Nobody can hear 64 parts per million in pitch. But in lab 8 the samples come from a computer whose clock runs at *its* idea of 48 000 Hz. Over a minute, 64 ppm adds up to about 184 extra samples. Lab 8 has to deal with that drift. `print_rates()` reads the real divider back out of the state machine's CLKDIV register, so you see the rate you actually got, not the one you asked for. (Stretch: pick a system clock that divides exactly.)

- **Signed samples.** All your DACs so far took **unsigned** codes, with silence in the middle (128, or 32 768). Audio chips use **signed** two's complement: silence is 0, and the 24-bit range is −8 388 608 … +8 388 607. C toolbox has the details. It matters for two things: the sine table holds `int32_t`, and moving a negative number into the top of a slot needs care (TODO 5).

- **dBFS: loudness relative to full scale.** 0 dBFS is the largest sine the format can hold. Every −20 dB divides the amplitude by 10, and every −6 dB roughly halves it:

  ```text
  gain = 10^(dB / 20)        V_out = 2.1 V rms × gain   (the PCM5102A's full scale)
  ```

  | Level | Gain | Output |
  |---|---|---|
  | 0 dBFS | 1 | 2.1 V rms |
  | −6 dBFS | 0.501 | 1.05 V rms |
  | −20 dBFS | 0.1 | 0.21 V rms |
  | −40 dBFS | 0.01 | 21 mV rms |

  For comparison, the ladder's 10 mV peak-to-peak into earbuds in lab 5 would be about −55 dBFS on this scale.

- **The output is centred on 0 V.** The PCM5102A has a **charge pump** that makes its own negative supply, so LOUT and ROUT swing **above and below ground**. That's the reason there's no 47 µF coupling capacitor this time. It's also a hazard: **never connect LOUT/ROUT to an RP2040 pin.** The ADC inputs can't take negative voltages, so this lab measures the output with the multimeter on AC volts, not with the ADC.

- **Earbuds are a hard load.** The PCM5102A's line driver is specified for loads of **1 kΩ or more**. Earbuds are 16–32 Ω. At low levels that's fine. Near full scale the driver runs out of current and clips, and 2.1 V rms in an earbud is painfully loud anyway. So: **earbuds only at −20 dBFS or below; −40 dBFS to start.** The final device puts a headphone amplifier after the DAC for exactly this reason.

- **Why DMA again?** At 48 kHz stereo, the 8-word FIFO holds only 4 frames = **83 µs**. In Part B the CPU feeds the PIO directly, as in lab 6, and anything that holds the CPU up for longer than 83 µs makes the PIO run dry. BCK stops, and the DAC mutes for a moment: a click. In Part C, DMA feeds the FIFO from two 5.3 ms buffers, exactly lab 4's ping-pong, except that the **PIO's own TX DREQ** paces it. No timer is needed: the FIFO asks for data whenever it has room.

| | PWM (lab 2) | R-2R (lab 5) | ΣΔ by hand (lab 6) | PCM5102A (this lab) |
|---|---|---|---|---|
| Pins | 1 | 8 | 1 | 3 |
| Resolution | 8 bits | 8 bits | 16-bit input, ~9 bits in audio | 24-bit input, ~18 bits real (112 dB, datasheet) |
| Sample rate | 20 kHz | 20 kHz | 20 kHz | 48 kHz |
| Stereo | (two slices) | no | no | yes |
| Output | 0…3.3 V, needs 47 µF | 0…3.3 V, needs 47 µF, 10 kΩ out | 0…3.3 V, needs 47 µF | ±3 V, centred on 0 V |

### C toolbox
New C in this lab. Shifts, masks, `|=`, `static`, `volatile`, casts and `&c` are in earlier toolboxes.

- **Two's complement.** A signed `int32_t` uses the same 32 bits as a `uint32_t`, but the top bit counts as **−2³¹** instead of +2³¹. So −1 is all ones, `0xFFFFFFFF`. In 24 bits:

  ```text
  value        24-bit pattern       hex        offset binary (labs 2–6 style)
  +8 388 607   0111 … 1111         0x7FFFFF   0xFFFFFF
           0   0000 … 0000         0x000000   0x800000   ← silence
          −1   1111 … 1111         0xFFFFFF   0x7FFFFF
  −8 388 608   1000 … 0000         0x800000   0x000000
  ```
  The two columns differ only in the top bit: `two's complement = offset binary ^ 0x800000`. Adding −1 + 1 with ordinary binary addition gives 0 with a carry out the top, which is exactly why CPUs use this format: one adder works for both.

- **Shifting signed numbers.** `sample << 8` with a negative `sample` is **undefined behaviour** in C: the standard doesn't say what happens. GCC usually does the obvious thing, but "usually" isn't good enough. Converting to unsigned first *is* defined (the value wraps modulo 2³², so −1 becomes `0xFFFFFFFF`), and shifting an unsigned number is always defined:

  ```c
  (uint32_t)sample << 8      // defined: the cast happens first, then the shift
  (uint32_t)(sample << 8)    // undefined for negative samples: the shift happens first
  ```
  The same trap catches small types: in `bit << 31` with a `uint16_t bit`, C first widens `bit` to a signed `int`, and shifting a 1 into the sign bit of an `int` is undefined too. Make `bit` a `uint32_t` and the problem goes away.

- **Building a word MSB first.** Lab 6 built a word LSB first: `word |= bit << n`. Going the other way, shift what you have up by one and drop the new bit in at the bottom:

  ```c
  word = (word << 1) | bit;    // after 32 rounds, the first bit is in bit 31
  ```

- **Pointer parameters.** `next_slot(uint32_t *phase, uint32_t inc)` updates whichever phase accumulator you point it at. It's the `&c` idea from lab 6, now in your own function:

  ```c
  next_slot(&phase_l, inc_l);   // inside: *phase means phase_l
  next_slot(&phase_r, inc_r);   // inside: *phase means phase_r
  ```
  Inside the function, `*phase` is "the variable `phase` points at". `*phase += inc;` reads it, adds `inc` and writes it back. Careful with `*phase++`: it means `*(phase++)`, which moves the *pointer*, not the value.

- **Integer division strikes again.** `LEVEL_DB / 20` with `LEVEL_DB` = −6 is `0` in integer arithmetic, so the gain comes out as 10⁰ = 1, full scale. That's a loud bug. Write `LEVEL_DB / 20.0f`, and the `int` is converted to `float` before dividing. `powf(10.0f, …)` is the `float` power function from `<math.h>`.

- **`_Static_assert(condition, "message")`.** A check done **by the compiler**: if the condition is false, the build stops with your message. The condition must be something the compiler can work out, like arithmetic on `#define`s. It catches wiring constants that don't fit together before anything is flashed.

- **Edge detection.** To find a rising edge, remember the previous level: `if (bck && !bck_was)` means "high now, low last time". Then set `bck_was = bck;` for next time. `gpio_get_all()` reads all 30 input levels in one go, so LRCK and DIN are sampled at the same instant as BCK.

- **Reading a register field.** CLKDIV keeps the integer part of the divider in bits 31…16 and the fraction in bits 15…8. `reg >> 16` brings the integer down to bit 0. `(reg >> 8) & 0xFF` brings the fraction down and masks off the integer above it. It's the C2/C3 skill, on a PIO register.

### Parts
- **PCM5102A module**: the common purple board with a 3.5 mm jack (`docs/bom.md` lists two; keep one spare). Its pin headers often come **loose**, so you may need to solder them, and maybe the configuration jumpers too (task 7.4). Two versions are common:
  - **Jumper version:** one 6-pin header (typically SCK, BCK, DIN, LCK, GND, VIN), and the four configuration pins set by solder pads on the back.
  - **Header version:** no pads. The configuration pins FLT, DMP, XMT and FMT come out to the header next to SCK (sometimes printed SCL), BCK, DIN and LCK, plus power pins VCC, 3.3V and GND. You set them with wires.

  Pin order differs between makers. Always go by the labels printed on your board.
- Short jumper wires, **10 cm or less** for BCK, LRCK and DIN
- Earbuds, multimeter
- Soldering iron, if the headers or jumpers need it

The base layout from part 1 (rails, 100 nF), and nothing else from earlier labs.

### Wiring
Rebuild the [base layout](labbook.md#prototype-board-base-layout) first: 3V3 to the red rails, GND to the blue rails, both rails bridged, 100 nF across them.

```text
   RP2040-Zero                          PCM5102A module
   ───────────                          ───────────────
   GP0  ──────────────────────────────  BCK
   GP1  ──────────────────────────────  LCK  (= LRCK)
   GP2  ──────────────────────────────  DIN
   GND rail ──────────────────────────  GND
   GND rail ──────────────────────────  SCK  (or SCL) ← no system clock: the PLL uses BCK
   5V pin  ── its own jumper ─────────  VIN  (or VCC) ← not via the rails
                                        3.3V          ← leave unconnected
                                        LOUT / ROUT / 3.5 mm jack → multimeter, earbuds

   Header version only (the jumper version sets these with pads, task 7.4):
   GND rail ──────────────────────────  FLT
   GND rail ──────────────────────────  DMP  (= DEMP)
   GND rail ──────────────────────────  FMT
   3V3 rail ──────────────────────────  XMT  (= XSMT)  ← forget this one and it stays muted
```

- **Put the module in zone B/C**, as close to the RP2040-Zero as it fits, so the three signal wires stay short. At 3 MHz, long loose wires ring, and the DAC can read a ringing edge as two.
- **VIN from the 5V pin.** The module has its own regulators and takes 3.3–5 V on VIN (check your board's silkscreen; if it says 3.3 V only, use the 3V3 rail instead). Run one dedicated jumper from the RP2040-Zero's 5V pin to VIN, and keep 5 V off the rails, as always. Why not via a rail? The rails feed 3.3 V-only parts, and RP2040 pins are **not 5 V tolerant**: one lead in the wrong hole of a 5 V rail could kill a GPIO. A single point-to-point wire is easy to check.
- **VCC is VIN.** The header version calls the regulator input VCC. To confirm, use the multimeter's continuity mode (power off) to check that VCC goes to a regulator's input pin. Its **3.3V** pin is that regulator's output. Leave it unconnected: tied to your 3V3 rail, two regulators would fight each other.
- **SCK to GND.** Some modules have a solder bridge on the front labelled SCK for this. Either is fine; don't leave SCK floating. The header version may print it as **SCL**.
- **Configuration pins (header version): never floating.** A floating input reads 0 or 1 at random and can change while running. FMT flipping, for example, would switch the format under your feet. XMT goes to the **3V3** rail, never to 5 V.
- **Never wire LOUT, ROUT or the jack to an RP2040 pin** (see Background).
- Do Part A **before** wiring the module. It only needs the bare board.

### Tasks

**7.1 Base wiring.** Rails, decoupling capacitor, USB cable, as in part 1. Nothing on GP0–GP2 yet.

**7.2 Build the untouched skeleton.** Pick `07-i2s-dac/lab07_i2s_dac` when you click Run. Parts A and B build with zero warnings. Part C warns that `phase_l`, `phase_r` and `next_slot` are unused until TODO 7. Flash Part A as it is. It prints *"only 0 BCK rising edges"*. Why? *(Look at the side-set values in `i2s.pio`.)*

**7.3 Part A: a logic analyser made of your own CPU** (`PART 0`, no module needed)
- **On paper first:** using the frame diagram in Background, write down for each of the 8 instructions in `i2s.pio` whether BCK and LRCK should be 0 or 1. Then work out the two loop counts: how many times must each loop body run, and so what goes in `set x, …`?
- TODO 1: the side-set values and loop counts. TODO 2: the state machine setup. The divider should print **62500 + 0/256**, which gives BCK = 1 kHz.
- TODO 3: `read_word()`.
- What you're aiming for:

  ```text
  LRCK low for 32 bits, then high for 32 bits (want 32 and 32)
             |left slot                           |right slot
  LRCK 0 |00000000 00000000 00000000 00000001 |11111111 11111111 11111111 11111110
  DIN  0 |10000000 00000000 00000000 00000001 |00010010 00110100 01010110 01111000
  left  0x80000001  sent 0x80000001  OK
  right 0x12345678  sent 0x12345678  OK
  ```
  Find the one-bit delay in the picture: which column has LRCK = 1 under a *left* bit?
- **Question:** the CPU reads the pins thousands of times per BCK period here. Could it do the same at the real 3.072 MHz? Roughly how many CPU cycles would it get per BCK period?

**7.4 Meet the module** (power off, USB unplugged)
- Look at both sides of the board and write down in Notes: the pin names on each header, the regulator(s), and how the four configuration pins are set. Which version is yours (see Parts)? Either way, they must end up like this:

  | # | Pin (header label) | Set to | Header version: wire to | Why |
  |---|---|---|---|---|
  | 1 | FLT | **L** | GND rail | normal-latency filter |
  | 2 | DEMP (DMP) | **L** | GND rail | de-emphasis off (an old CD feature) |
  | 3 | XSMT (XMT) | **H** | 3V3 rail | soft-mute *off*. Low = muted. A silent module is usually this one. |
  | 4 | FMT | **L** | GND rail | I2S format. H = left-justified. |

- **Jumper version:** the pads are on the back, numbered **1–4**, each a little middle pad between an **H** and an **L** pad. If they aren't bridged, bridge each middle pad to the side in the table with a blob of solder.
- **Header version:** wire each pin as in the table. Don't leave any of them floating (see Wiring).
- If your board has pads *and* header pins, **use one way, not both**.
- Wire the module as in Wiring. Check twice before plugging in USB: VIN/VCC goes to the 5V pin, the 3.3V pin is unconnected, GND is connected, SCK/SCL goes to GND, and (header version) the four configuration wires are in place.

**7.5 Part B: a 48 kHz stereo sine** (`PART 1`)
- TODO 4: the gain. TODO 5: `to_slot()`. TODO 6: `next_slot()`.
- Check the printout: divider **20 + 88/256**, sample rate **48003.07 Hz**.
- Earbuds in through the module's jack at −40 dBFS. The **left ear should be lower (440 Hz)** and the right higher (660 Hz, a fifth above). Tuner app on each ear.
- **Earbuds out**, multimeter on **AC volts** between LOUT and AGND (or the jack's tip and sleeve). Fill in the level table in Notes for −20, −6 and 0 dBFS. Does every 20 dB step divide the reading by 10?
- Watch the stall count. Does it stay at 0?

**7.6 Part C: DMA feeds the PIO** (`PART 2`)
- TODO 7: `fill_buffer()`. TODO 8: the two DMA channels.
- It should sound exactly like Part B. Stalls and underruns stay at **0**.
- **Question:** in lab 4, a DMA *timer* paced the transfers. What paces them here, and why is that better for audio?

**7.7 Break it on purpose**
- **Print storm:** set `PRINT_MS` to 5 and run Part B (earbuds in, −40 dBFS). Listen and watch the stall count. Then run Part C with the same setting. Why does only one of them click?
- **Forget the shift:** in TODO 5, return `(uint32_t)sample` without `<< 8`. Predict how many dB quieter it gets (each bit is 6.02 dB), then measure at 0 dBFS with the earbuds **out**. Put the shift back.
- **Paper only, don't play it:** what would the DAC output if you sent lab 6's style of *offset* samples (0 … 16 777 215, silence = 8 388 608) instead of signed ones? Sketch one cycle of the waveform the DAC would make from a sine. (Hint: two's complement column in the C toolbox.)

### Hints (read only if stuck)
<details><summary>TODO 1: the I2S program</summary>

```text
.wrap_target
    set x, 30           side 0b01   ; BCK high: DAC reads the right LSB. LRCK: left from now on
left_bits:
    out pins, 1         side 0b00   ; BCK low: next left bit onto DIN
    jmp x-- left_bits   side 0b01   ; BCK high: DAC reads it
    out pins, 1         side 0b10   ; BCK low: the left LSB, but LRCK already says right
    set x, 30           side 0b11   ; BCK high: DAC reads the left LSB
right_bits:
    out pins, 1         side 0b10   ; BCK low: next right bit onto DIN
    jmp x-- right_bits  side 0b11   ; BCK high: DAC reads it
    out pins, 1         side 0b00   ; BCK low: the right LSB, but LRCK already says left
.wrap
```
Each loop sends bits 31 … 1 of a slot: 31 bits, so x starts at 30. The `out` after each loop sends bit 0 with LRCK already flipped: that's the one-bit delay. Count the instructions per frame: 1 + 31 × 2 + 1 + 1 + 31 × 2 + 1 = 128 = 64 bits × 2.
</details>

<details><summary>TODO 2: state machine setup</summary>

```c
sm_config_set_out_pins(&c, I2S_DIN_PIN, 1);
sm_config_set_sideset_pins(&c, I2S_BCK_PIN);
sm_config_set_out_shift(&c, false, true, 32);   // shift LEFT: bit 31 goes out first
sm_config_set_clkdiv(&c, (float)clock_get_hz(clk_sys) / pio_hz);
```
Delete `(void)pio_hz;`. The number of side-set pins (2) comes from `.side_set 2`, and `i2s_out_program_get_default_config()` has already put it into `c`. You only say where they start.
</details>

<details><summary>TODO 3: read_word</summary>

```c
for (uint32_t k = 0; k < 32; k++) {
    word = (word << 1) | cap_din[start + k];
}
```
Delete `(void)start;`.
</details>

<details><summary>TODO 4: the gain</summary>

```c
float gain = powf(10.0f, LEVEL_DB / 20.0f);
printf("%d dBFS: gain %.5f, expect %.3f V rms\n", LEVEL_DB, gain, 2.1f * gain);
```
</details>

<details><summary>TODO 5: to_slot</summary>

```c
return (uint32_t)sample << 8;
```
Delete `(void)sample;`.
</details>

<details><summary>TODO 6: next_slot</summary>

```c
*phase += inc;
return to_slot(sine_table[*phase >> (32 - TABLE_BITS)]);
```
Lab 3's two lines, with `*phase` instead of `phase`. Delete the two `(void)` lines.
</details>

<details><summary>TODO 7: fill_buffer</summary>

```c
for (uint32_t f = 0; f < BUF_FRAMES; f++) {
    buf[2 * f] = next_slot(&phase_l, inc_l);
    buf[2 * f + 1] = next_slot(&phase_r, inc_r);
}
```
Delete `(void)buf;`. Left must come first: the PIO starts every frame with the left slot.
</details>

<details><summary>TODO 8: DMA into the PIO</summary>

```c
channel_config_set_dreq(&c, pio_get_dreq(I2S_PIO, sm, true));     // true = the TX FIFO
channel_config_set_chain_to(&c, dma_chan[1 - b]);
dma_channel_configure(dma_chan[b], &c,
                      &I2S_PIO->txf[sm],      // write here: the TX FIFO
                      buffers[b],             // read from here
                      2 * BUF_FRAMES, false); // this many words, don't start yet
```
Delete `(void)c;`. Lab 4's TODO 5, with the PWM register swapped for the FIFO and the timer's DREQ swapped for the PIO's.
</details>

### Stretch challenges
- [ ] **An exact 48 kHz:** call `set_sys_clock_khz(153600, true);` as the first line of `main()`. 153.6 MHz / 6.144 MHz = 25 exactly, so the divider has no fraction. Does `print_rates()` agree? (153.6 MHz is within what current RP2040 datasheets and SDK 2.x allow.)
- [ ] **44.1 kHz:** what divider would CD audio need at 125 MHz? How far off is the nearest 1/256 step, in ppm?
- [ ] **Left-justified:** set FMT to H and change the program so LRCK flips together with the MSB, not one bit early. Compare with `firmware/rp2040/src/i2s.pio`. Then work out on paper what the DAC reads when FMT and the program **disagree**, both ways round. One mismatch sounds almost innocent (just 6 dB louder at low levels); the other badly distorts every negative sample. Which is which? Try it only at −40 dBFS.
- [ ] **16-bit slots:** 32 BCK periods per frame, with a left and a right 16-bit sample packed into one 32-bit word. Change the loop counts, `SLOT_BITS` and the packing. Which half of the word goes out first?
- [ ] **Volume knob in software:** multiply each sample by a gain while it plays (in `int64_t`, then shift back down). Change the gain suddenly, then ramp it over 10 ms. Hear the click go away?
- [ ] **All four DACs:** play 440 Hz through PWM (lab 3), the ladder (lab 5), sigma-delta (lab 6) and the PCM5102A, and write one line about each in Notes. Is the PCM5102A's background silent at −40 dBFS?

### Checks
- [ ] Untouched Part A finds no BCK edges, and I know why
- [ ] Part A: LRCK low 32 / high 32, both test words OK
- [ ] Found the one-bit delay in the Part A picture
- [ ] Module pins and configuration (pads or wires) recorded in Notes; FLT L, DEMP L, XSMT H, FMT L
- [ ] Part B: 440 Hz left, 660 Hz right, checked with the tuner
- [ ] Level table: −20, −6 and 0 dBFS within about 10 % of 0.21 / 1.05 / 2.1 V rms
- [ ] Sample rate prints 48003.07 Hz, and I can explain the 64 ppm
- [ ] Part C: 0 stalls and 0 underruns, even with `PRINT_MS 5`
- [ ] Can explain why it's `(uint32_t)sample << 8` and not `sample << 8`
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:*

*Module: version (jumper / header), pins on each header, regulators, configuration as found / as set:*

*Part A printout:*

*Divider and sample rate printed (Part B):*

*Levels (earbuds out, multimeter AC V between LOUT and AGND):*

| LEVEL_DB | Expected V rms | Measured | Measured / previous |
|---|---|---|---|
| −20 | 0.21 | | — |
| −6 | 1.05 | | |
| 0 | 2.1 | | |

*Part B vs Part C with `PRINT_MS 5`: stalls, what it sounded like:*

*Without the `<< 8`: predicted dB, measured:*

*All four DACs compared:*

*Questions for next time:*

---

## Lab 8 — USB audio → PCM5102A

### Goal
Turn the board into a **USB sound card**. The computer finds a device called *signaldeck USB DAC*, you pick it in Sound settings, and music plays through lab 7's I2S and DMA into the PCM5102A. On the way you write the parts of a USB audio device that matter, then solve the problem every USB DAC has: the computer's clock and yours don't run at quite the same speed. The fix is the **feedback endpoint** and a small control loop.

### Background
- **Where this fits.** Lab 7 played samples the RP2040 made itself. Now they come from outside, 48 000 times a second per ear, from a computer that has its own idea of what a second is. The I2S side doesn't change at all: your `i2s.pio`, your state machine setup and your DMA ping-pong are copied into `main.c` as they are. What's new is everything in front of them:

  ```text
  computer ──USB──► TinyUSB ──► FIFO ──► fill_from_usb() ──► DMA buffers ──► PIO ──► PCM5102A
           packets   (driver)   (bytes)   unpack, attenuate    lab 7           lab 7
                                   ▲
                                   └── how full is it? ──► feedback ──USB──► computer: "send a bit more / less"
  ```

- **USB in one page.** The computer is the **host** and the board is a **device**. The host decides everything: the device only ever answers. The RP2040 talks **full speed**, 12 Mbit/s. Time on the bus is cut into **frames** of exactly **1 ms**. Each one starts with a SOF ("start of frame") packet from the host, so the host's clock sets the rhythm.

  Data travels through **endpoints**: numbered mailboxes, each with a direction. Seen from the host, **OUT** goes to the device and **IN** comes back from it. Bit 7 of the endpoint address is the direction: `0x01` is EP1 OUT, `0x81` is EP1 IN. Endpoint 0 is special: every device has it, and the host uses it for **control requests** ("who are you?", "what's your sample rate?"). The other endpoints each have a **transfer type**:

  | Type | Guarantee | Used for |
  |---|---|---|
  | Control | delivered, checked, retried | setup and questions (EP0) |
  | Bulk | delivered eventually, when the bus is free | the serial port, flash drives |
  | Interrupt | small, polled regularly | keyboards, mice |
  | **Isochronous** | **a reserved slot in every frame**, never retried | **audio**, video |

  Audio uses isochronous: if a packet is damaged, resending it would only make it late, and a late sample is as useless as a lost one.

- **Enumeration: the device introduces itself.** When you plug the board in, the host asks for its **descriptors**: tables of bytes that describe the device. TinyUSB answers by calling the `tud_descriptor_..._cb()` functions in `usb_descriptors.c`. The descriptors form a tree:

  ```text
  device descriptor          "USB 2.0, vendor 0xCAFE, product 0x4008, 1 configuration"
  └─ configuration           "5 interfaces, up to 100 mA"
     ├─ IAD: audio function (interfaces 0–1)
     │  ├─ interface 0  AudioControl:   clock source, input terminal, output terminal
     │  └─ interface 1  AudioStreaming
     │     ├─ alt 0  idle: no endpoints, no bandwidth
     │     └─ alt 1  streaming: format 24-bit stereo
     │        ├─ EP 0x01 OUT  isochronous: the samples
     │        └─ EP 0x81 IN   isochronous: the feedback
     ├─ IAD: serial port (interfaces 2–3)  → printf
     └─ interface 4  vendor: picotool's reset button
  string descriptors         "signaldeck", "signaldeck USB DAC", serial number …
  ```
  An **interface** is one function of the device, and its **class** tells the host which driver to load. Your board has three functions at once: a sound card (UAC2, USB Audio Class 2), a serial port (CDC) and the reset interface the Run button uses. A device like this is called **composite**. An **IAD** (interface association descriptor) groups the interfaces that belong to one function. In labs 0–7 the SDK quietly provided the serial port's descriptors. Now that you have your own, the SDK steps back, and `usb_descriptors.c` lists all three.

- **UAC2: the audio function as a little circuit.** Inside the AudioControl interface, the descriptors draw a graph of **entities**, connected by ID numbers:

  ```text
  USB stream ──► input terminal (ID 1) ──► output terminal (ID 3): headphones
                        └─────── both clocked by ───────┘
                                clock source (ID 4)
  ```
  The host talks to the entities with **control requests**: `GET` or `SET`, for a **control** (for the clock: *sample rate* or *clock valid*), with an **attribute**: **CUR** (the current value) or **RANGE** (the allowed values). Before it plays anything, a host asks the clock something like:

  ```text
  GET clock, sample rate, RANGE  → "which rates can you do?"     48 000 … 48 000, step 0
  GET clock, sample rate, CUR    → "which rate are you at?"      48 000
  GET clock, clock valid, CUR    → "is your clock stable?"       1
  ```
  If the device refuses (a **stall**), the host gives up on the function. That's TODO 2. A real DAC would add a *feature unit* between the terminals for volume and mute. This one leaves it out on purpose: volume will be an analog knob (`docs/architecture.md`). So the volume slider in your Sound settings stays greyed out.

- **Alternate settings: bandwidth on demand.** Isochronous endpoints reserve bus time in every frame, whether or not anything is playing. So the streaming interface comes in two versions. **Alt 0** has no endpoints at all. **Alt 1** has the two audio endpoints. When an app starts playing, the host switches to alt 1 (`tud_audio_set_itf_cb()`), and some time after it stops, back to alt 0. Your event log shows both.

- **What a packet looks like.** In each 1 ms frame, the host sends one packet: normally 48 frames of audio (48 000 / 1000), **interleaved** L R L R, each sample **3 bytes**, **least significant byte first** ("little-endian"):

  ```text
  byte:   0    1    2    3    4    5    6    7    8   …  285  286  287
         [L0   L1   L2 ][R0   R1   R2 ][L0   L1   L2 ] …  [R0   R1   R2 ]
          └─ left, frame 0 ┘└─ right, frame 0 ┘           └─ right, frame 47 ┘
          L0 = bits 7…0, L1 = bits 15…8, L2 = bits 23…16 (with the sign bit)
  48 frames × 2 samples × 3 bytes = 288 bytes per packet
  ```
  The descriptors announce this: **3 bytes per sample** (the "subslot size") and **24 bits used** (the "resolution"). They also give the **biggest packet** the host may ever send. More on why it can be bigger than 288 below. That's TODO 1.

- **Two clocks, one tank.** The host sends samples at *its* 48 kHz, counted by *its* crystal. Your PIO plays them at *your* 48 003.07 Hz (lab 7), counted by *your* crystal. Crystals are only accurate to some tens of ppm, and you're 64 ppm fast on top of that. In between sits TinyUSB's **FIFO**, like a water tank: packets pour in at the top, `fill_from_usb()` drains it from the bottom.

  ```text
         host: 48 000.0 frames/s  (by the host's clock)
                    │
               ┌────▼────┐
               │ ~~~~~~~ │ ← the level: frames waiting
               │ ~~~~~~~ │   start at half full (TARGET_FRAMES)
               └────┬────┘
                    │
         you: 48 003.07 frames/s  (by your clock)
  ```
  If the two rates differ by 64 ppm, the level changes by 48 000 × 64 × 10⁻⁶ ≈ **3 frames per second**. Your 784-frame tank, started half full, then runs dry in about two minutes, or overflows if the error goes the other way. Either way you hear a dropout, then again two minutes later, forever. Part B makes you watch it happen.

- **Three ways out.** USB audio defines three kinds of isochronous stream:
  - **Synchronous:** the device locks its clock to the host's 1 ms SOF. That needs a tunable clock (a PLL you can nudge), and SOF jitter leaks into the audio.
  - **Adaptive:** the device measures how fast data actually arrives and tunes its clock to match. Same need for a tunable clock.
  - **Asynchronous:** the device keeps its own clean, fixed clock and **tells the host** how fast it really plays. The host then sends slightly more or fewer samples. This is what good USB DACs do, and what your descriptors say (`TUSB_ISO_EP_ATT_ASYNCHRONOUS`). The PIO clock never changes, which is good for the sound: the DAC's clock stays clean.

- **The feedback value.** Every frame, the host reads a number from the feedback endpoint: **how many samples per channel you consume per 1 ms frame**. It isn't a whole number. You consume 48.00307. So it's sent in **fixed point**, like lab 3's phase. TinyUSB takes it in **16.16** format: the top 16 bits are the whole part, the bottom 16 the fraction:

  ```text
  value = samples per frame × 65 536
  48.0     → 48 × 65 536                 = 3 145 728
  48.00307 → 48.00307 × 65 536 ≈ 3 145 929     (201 more: the 64 ppm)
  ```
  The host keeps a running total of these fractions, and whenever the total gains a whole sample it sends a **49-sample packet** instead of 48. That's why the biggest packet in TODO 1 has room for one extra frame. Full-speed USB actually specifies a 3-byte **10.14** format on the wire. TinyUSB converts for you when `CFG_TUD_AUDIO_ENABLE_FEEDBACK_FORMAT_CORRECTION` is 1, which macOS needs. Windows wants 16.16 as it is (Stretch).

- **You can't compute the right value; you have to measure it.** Part B tries the obvious thing: send 48.00307, your exact rate (TODO 6b). The drift slows down, but doesn't stop. Your 48 003.07 is measured with *your* crystal, but the host counts its frames with *its* crystal, and the two disagree by some ppm you don't know. What you *can* see is the FIFO level. If it falls, you're consuming faster than the host sends, so ask for more. If it rises, ask for less. That's a **control loop**, like a thermostat:

  ```text
  error    = TARGET_FRAMES − level          (positive: too empty)
  feedback = feedback_base + KP × error     (more than base: "send me more")
  ```
  This is a **proportional (P) controller**: the correction grows with the error. With KP = 8 units per frame, a 100-frame error asks for 800 / 65 536 ≈ 0.012 extra samples per frame, which is 12 frames per second, so the level comes back in seconds. A P controller settles a little **off** target: it needs some error to keep producing the small correction for the crystal mismatch. (Stretch: an integral term removes that.)

- **Smoothing the level.** The raw level jumps around. A 48-frame packet arrives every 1 ms, and you take 96 frames every 2 ms, so depending on timing the level you read swings by about 48 frames, even when nothing is drifting. Feed that straight into the controller and the feedback jitters too. An **exponential moving average** smooths it:

  ```text
  avg = avg + (level − avg) / 64          each time a buffer is filled
  ```
  Each new reading moves the average 1/64 of the way towards it. Noise averages out, a real trend still comes through, and it costs one subtraction and one division. TODO 5 does it in 16.16 fixed point, so the average keeps a fraction of a frame.

- **TinyUSB: callbacks and `tud_task()`.** TinyUSB is the USB stack the Pico SDK ships. You don't call it to handle a request. It calls **you**, through functions with fixed names: `tud_mount_cb()`, `tud_audio_set_itf_cb()`, `tud_audio_get_req_entity_cb()` and so on. Two rules follow:
  - **Call `tud_task()` often.** On the RP2040, finished USB transfers are only handled inside `tud_task()`, including copying each audio packet into the FIFO. The main loop calls it on every pass. Hold the loop up for several milliseconds and packets are lost (task 8.7).
  - **Never `printf` in a callback.** Callbacks run *inside* `tud_task()`, and `printf` over USB calls `tud_task()` itself to push the text out. A callback that printed would re-enter TinyUSB in the middle of its own work. So the callbacks write a small **event** into a list, and the main loop prints it later (`log_event()`, `print_events()`).

- **No volume control yet, so mind your ears.** Music is mastered to peak near **0 dBFS**, which is 2.1 V rms out of the PCM5102A: far too loud for earbuds, and enough to make the line driver clip into them (lab 7). Until the analog volume knob exists, `OUTPUT_SHIFT` attenuates in software, **−6 dB per step**, and starts at 4 (−24 dB). Also keep the app's own volume low to start. With speakers you can use 0–2.

### C toolbox
New C in this lab. Casts before shifts, two's complement, pointer parameters and `_Static_assert` are in lab 7's toolbox.

- **Bytes into a word, little-endian.** The wire gives you `p[0]`, `p[1]`, `p[2]`, least significant first. Building a number from bytes is shifts and ORs. Each byte is cast up to `uint32_t` *before* it's shifted, for the same reason as lab 7's `to_slot()`: a `uint8_t` is promoted to a signed `int` first, and `p[2] << 24` could shift a 1 into an `int`'s sign bit, which is undefined:

  ```c
  ((uint32_t)p[1] << 8) | p[0]     // two bytes, little-endian → a 16-bit value
  ```
  Where must each byte land so the sample ends up in bits 31…8, as lab 7's slots want? Note that you don't need to **sign-extend**. The sign bit, bit 7 of `p[2]`, lands in bit 31, which is exactly where a slot keeps it.

- **Structs and designated initializers.** A `struct` groups named fields into one type. TinyUSB has a struct for every control answer:

  ```c
  audio_control_cur_4_t rate = { .bCur = 48000 };   // one field, called bCur
  ```
  `.bCur = …` is a **designated initializer**: it sets fields by name, and any field you don't mention becomes 0. Nested structs and arrays work the same way: `.subrange[0] = { .bMin = …, .bMax = … }`. `audio_control_range_4_n_t(1)` is a macro that *makes* a struct type with room for 1 subrange. TinyUSB marks these structs **packed**: no padding bytes between fields, because the bytes go straight onto the wire. The RP2040 and USB are both little-endian, so the numbers need no byte swapping.

- **One set of bytes, a different struct.** `tud_audio_get_req_entity_cb()` receives a generic `tusb_control_request_t *`. For an audio request, the same 8 bytes have audio-specific meanings, so the code casts the pointer: `(audio_control_request_t const *)p_request`. No bytes move. The compiler just reads them through a different struct layout. `const` means "I promise not to change what this points at".

- **Callbacks with fixed names, and weak functions.** You never tell TinyUSB about `tud_audio_set_itf_cb()`: it already calls a function with that exact name. TinyUSB contains a default version of each optional callback, marked **weak**. If you write one with the same name, the linker uses yours instead. That's why a typo in a callback's name isn't an error: your function is simply never called, and the weak default runs.

- **`enum`, `typedef` and compound literals.** `typedef enum { EV_MOUNT, EV_UNMOUNT, … } event_type_t;` gives names to the numbers 0, 1, 2…, and `typedef` makes `event_type_t` a type name. `typedef struct { … } event_t;` does the same for a struct. `(event_t){type, entity, …}` is a **compound literal**: a struct value made on the spot, with its fields in order. `log_event()` assigns it in one line.

- **A ring buffer with free-running counters.** The event list is an array of 32, with two counters that only ever go up: `ev_written` and `ev_printed`. The slot for counter `n` is `n & (EVENT_COUNT − 1)`, which is `n % 32` done with a mask because 32 is a power of 2. `ev_written − ev_printed` is how many events are waiting. That subtraction stays correct even when the counters wrap past 2³², thanks to unsigned arithmetic (lab 3).

- **Fixed point, again.** The feedback is 16.16, so 1.0 = `1 << 16` = 65 536. To turn a `double` into it, multiply by 65 536 and round to the nearest whole number. `lround()` from `<math.h>` returns a `long`. The averaged level is 16.16 too: `level << 16` turns whole frames into 16.16, and `avg >> 16` turns it back (rounding down).

- **Signed steps in unsigned variables.** The moving average's step, `(level − avg) / 64`, is negative whenever the level falls. With `uint32_t`, "negative" wraps around to about 4 billion. Do the step in `int32_t`: cast both sides to `int32_t`, subtract, divide, then add the step back. 784 frames × 65 536 is about 51 million, far below the `int32_t` limit of about 2.1 billion, so the casts are safe here.

- **Why `attenuate()` divides instead of shifting right.** For negative numbers, `>>` is **implementation-defined** in C: GCC copies the sign bit in (an "arithmetic" shift), but the standard doesn't promise it. Division by a power of 2 is always defined. It rounds towards zero, which is close enough for a volume control. The cast to `int32_t` first matters: it makes the top bit count as the sign, so the divided slot stays negative.

- **`#if` and `#error`.** The preprocessor can do arithmetic on `#define`s: `#if CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX == 0` checks a value before anything is compiled, and `#error "…"` stops the build with that message. `tusb_config.h` uses this to stop the untouched skeleton at TODO 1, instead of letting it build a device with a 0-byte format.

- **`printf("%-9s", …)`.** A width pads the text to 9 characters, and `-` puts the padding on the right. It keeps the status lines lined up.

### Parts
Lab 7's breadboard, unchanged: the RP2040-Zero, the PCM5102A module with its configuration set, and your external jack on L/G/R. Plus:
- Earbuds or powered speakers
- A music player on the computer, or a website that plays test tones
- A USB **data** cable (as always)

### Wiring
Nothing new. Check that lab 7's wiring is still there: GP0 → BCK, GP1 → LCK, GP2 → DIN, the 5V pin → VIN/VCC on its own jumper, GND, SCK to GND, and the configuration pins set.

### Tasks

**8.1 A map of the skeleton.** Lab 8 is several files. Open each one and read its top comment:

| File | What's in it | TODOs |
|---|---|---|
| `tusb_config.h` | TinyUSB settings: which classes, the stream format, buffer sizes | 1 |
| `usb_descriptors.h` | interface numbers, entity IDs, endpoint addresses | — |
| `usb_descriptors.c` | the descriptor tables and the name strings | — |
| `i2s.pio` | your lab 7 program, unchanged | — |
| `main.c` | your lab 7 I2S + DMA, TinyUSB callbacks, the audio path | 2–7 |

Find the descriptor tree from Background in `desc_configuration[]` in `usb_descriptors.c`: each `TUD_…` macro is one branch.

**8.2 Build the untouched skeleton.** Pick `08-usb-audio/lab08_usb_audio` when you click Run. The build **stops** with *"TODO 1 (tusb_config.h): fill in the stream format"*, plus a few follow-on warnings. That's on purpose: a sound card with a 0-byte format makes no sense.

**8.3 Part A: the computer finds a sound card** (`PART 0`)
- TODO 1: the format. All of it can be worked out from Background: the packet layout, and why the host may send one extra frame.
- Build and flash, with TODO 2 still untouched. Open the serial monitor (the port is still called `usbmodem…`). You should see the banner, `USB: configured by the host`, and some `GET clock …` lines. What does the host get back? What does macOS's **Audio MIDI Setup** app show for *signaldeck USB DAC*?
- TODO 2: the clock's answers. Flash again. Now Audio MIDI Setup should show **2 ch 24-bit Integer 48.0 kHz**. Write down in Notes the order of the requests you see.
- Pick *signaldeck USB DAC* as the output in System Settings → Sound, and play something. You won't hear anything: Part A throws the samples away. What do the events say when you press play, and some time after you stop? What happens to the FIFO level, and why does it stop where it stops?
- **Question:** the volume slider for this device is greyed out. Which descriptor would you have to add to enable it?

**8.4 Part B: play, and watch the clocks drift** (`PART 1`, `FEEDBACK_START 0`)
- TODO 3: `unpack_slot()`. TODO 4: `fill_from_usb()`. TODO 5: `update_level_avg()`. TODO 6a: `FEEDBACK_NOMINAL`.
- Check the banner: `feedback 3145728 = 48.00000`.
- Earbuds in (or speakers), `OUTPUT_SHIFT 4`, the music app's volume low. Play music. It should sound right, and the status line should say `playing`.
- Now leave a long track or a test tone playing and **don't touch anything**. Write down the FIFO level every 30 s for 3 minutes. Which way does it go, and how fast in frames per second? Turn that into ppm: `ppm = (frames per second) / 48 000 × 10⁶`. Compare it with lab 7's +64 ppm. If you wait long enough, you'll hear a dropout and see `short` (or `overflow`) count up.
- **Question:** why does `service_audio()` wait for the FIFO to be half full before it starts playing, and go silent until it's half full again after running dry?

**8.5 Part B again: the exact rate isn't enough** (`FEEDBACK_START 1`)
- TODO 6b: `feedback_from_rate()`. The banner should now show about **3145929 = 48.00307**.
- Repeat the 3-minute measurement. The drift should be much slower, but probably not zero. Work out the remaining ppm. Where does it come from? (Background: "You can't compute the right value".)

**8.6 Part C: close the loop** (`PART 2`, `FEEDBACK_START 1`)
- TODO 7: `feedback_control()`.
- Play for **10 minutes**. The level should settle near `TARGET_FRAMES` (392), a few frames off, and stay there. `short` and `overflow` should stay at **0**.
- Watch the feedback value in the status line. It should settle a little away from 48.00307. From that, work out the crystal mismatch between your computer and your board in ppm. Does it match what you measured in 8.5?
- **Question:** predict the steady-state error in frames from your mismatch and `KP`, then compare with what you see.

**8.7 Break it on purpose**
- **Greedy controller:** set `KP` to 2000. Does the level still settle, or does it swing? Why does a much faster controller get worse, when the FIFO level and the host's reaction both lag behind? Put it back to 8.
- **Starve TinyUSB:** add `sleep_ms(5);` at the end of the main loop in Part C. What do you hear, and which counters move? Explain it with the "call `tud_task()` often" rule, and with what DMA buffers of 2 ms mean for `late`. Remove it.
- **Wrong byte order (paper first):** what would you hear if `unpack_slot()` took the bytes the wrong way round, `p[0]` as the most significant? Work out what the quietest positive sample, +1 (bytes `01 00 00`), would become, and how many dB louder that is. Then try it briefly, **speakers only, at a low volume**: no earbuds.

**8.8 Listen**
- Play music you know well, at a comfortable level. Compare it with the computer's own headphone output. Note what you hear in Notes.
- **Question:** the whole path from the app to your ear adds some delay. Add up the parts you control: the FIFO at its target, plus the two DMA buffers. How many milliseconds is that?

### Hints (read only if stuck)
<details><summary>TODO 1: the format</summary>

3 bytes per sample, 24 bits of resolution. The biggest packet is one frame more than the normal 48:

```c
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX  3
#define CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX          24
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX \
    ((AUDIO_SAMPLE_RATE_HZ / 1000 + 1) * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX)
```
(48 + 1) × 2 × 3 = 294 bytes. A `#define` that continues on the next line needs a `\` at the end of the line.
</details>

<details><summary>TODO 2: the clock's answers</summary>

`.bCur = AUDIO_SAMPLE_RATE_HZ` for the rate. For the range, `.wNumSubRanges = 1`, and the one subrange has `.bMin` and `.bMax` both `AUDIO_SAMPLE_RATE_HZ`, with `.bRes = 0`. For "clock valid", `.bCur = 1`. Untouched, the host is told that the rate is 0 Hz and the clock isn't valid, so it won't use the device.
</details>

<details><summary>TODO 3: unpack_slot</summary>

```c
return ((uint32_t)p[2] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[0] << 8);
```
</details>

<details><summary>TODO 4: fill_from_usb</summary>

```c
uint32_t frames = tud_audio_read(bytes, sizeof(bytes)) / BYTES_PER_FRAME;
for (uint32_t f = 0; f < frames; f++) {
    const uint8_t *p = &bytes[f * BYTES_PER_FRAME];
    buf[2 * f]     = attenuate(unpack_slot(p));                     // left
    buf[2 * f + 1] = attenuate(unpack_slot(p + BYTES_PER_SAMPLE));  // right
}
for (uint32_t i = 2 * frames; i < 2 * BUF_FRAMES; i++) buf[i] = 0;
return frames;
```
Delete the `(void)` lines. `p + BYTES_PER_SAMPLE` is a pointer 3 bytes further on: the right sample of the same frame.
</details>

<details><summary>TODO 5: update_level_avg</summary>

```c
int32_t step = ((int32_t)(level_frames << 16) - (int32_t)level_avg) / 64;
level_avg = (uint32_t)((int32_t)level_avg + step);
```
</details>

<details><summary>TODO 6: feedback values</summary>

```c
#define FEEDBACK_NOMINAL    ((uint32_t)(AUDIO_SAMPLE_RATE_HZ / 1000) << 16)
```
```c
return (uint32_t)lround(rate_hz / 1000.0 * 65536.0);
```
</details>

<details><summary>TODO 7: feedback_control</summary>

```c
int32_t error = TARGET_FRAMES - (int32_t)(level_avg >> 16);
int32_t fb = (int32_t)feedback_base + KP * error;
int32_t max = (int32_t)(FEEDBACK_NOMINAL + FEEDBACK_RANGE);
int32_t min = (int32_t)(FEEDBACK_NOMINAL - FEEDBACK_RANGE);
if (fb > max) fb = max;
if (fb < min) fb = min;
return (uint32_t)fb;
```
Too empty means a positive error, which makes the feedback bigger: "send me more".
</details>

### Stretch challenges
- [ ] **PI controller:** add an integral term, `integral += error` each fill, `feedback += KI × integral` (with KI much smaller than KP, and a limit on `integral`). The steady-state error should go to 0. Why?
- [ ] **44.1 kHz:** offer 44 100 and 48 000 in the RANGE answer, accept both in `tud_audio_set_req_entity_cb()`, and switch the PIO divider (and the nominal feedback) when the host picks one. 44.1 samples per frame isn't a whole number: what does the host do about the 0.1?
- [ ] **Windows:** set `CFG_TUD_AUDIO_ENABLE_FEEDBACK_FORMAT_CORRECTION` to 0 and try the board on a Windows PC. Then work out what macOS would make of 16.16 read as 10.14.
- [ ] **A volume slider:** add a feature unit between the terminals (TinyUSB's `uac2_speaker_fb` example has one), answer its volume and mute requests, and apply the volume in `attenuate()`.
- [ ] **Promote it:** lab 8 is the device's firmware. Replace the draft in `firmware/rp2040` with it: its own `CMakeLists.txt`, Part C only, no `PART` switch.

### Checks
- [ ] Untouched skeleton stops at TODO 1, and I know why
- [ ] With TODO 2 untouched, I saw what the host does when the clock refuses to answer
- [ ] Audio MIDI Setup shows *signaldeck USB DAC*: 2 ch, 24-bit, 48 kHz
- [ ] Request order and alt 0/1 switching recorded in Notes
- [ ] Part B plays music; drift measured with nominal feedback, in frames/s and ppm
- [ ] Drift measured again with the computed feedback; remaining ppm explained
- [ ] Part C: 10 minutes, level near 392, 0 short and 0 overflow
- [ ] Crystal mismatch worked out from the settled feedback value
- [ ] Can explain why callbacks must not `printf`
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:*

*Host and OS (e.g. MacBook, macOS version):*

*Part A: what the host did with TODO 2 untouched; request order once it worked; when alt 1 / alt 0 came:*

*Part B, nominal feedback: level every 30 s, drift in frames/s and ppm:*

| Time | 0:00 | 0:30 | 1:00 | 1:30 | 2:00 | 2:30 | 3:00 |
|---|---|---|---|---|---|---|---|
| Level | | | | | | | |

*Part B, computed feedback (48.00307): drift in frames/s and ppm:*

*Part C: settled level, settled feedback, crystal mismatch in ppm, predicted vs seen steady-state error:*

*Break it: KP 2000, sleep_ms(5), wrong byte order:*

*Listening, and the delay you added up:*

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
