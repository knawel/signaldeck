# signaldeck C Labbook: bits, registers and the datasheet

Three short labs with no audio. Their job is to make register code feel normal: `1u << 29`, `& ~mask`, `*(volatile uint32_t *)0xD000001C`, `sio_hw->gpio_togl`. Labs 4 and 5 of the [main labbook](labbook.md) already used all of these through the SDK. Here you write them yourself, **starting from the datasheet**: every address, offset and bit number in C2 is one you look up.

- **C1** is pure C on plain numbers, with PASS/FAIL checks like a tiny pytest. Then you look up your first two registers in the datasheet, read them and decode them.
- **C2** starts with a datasheet hunt of 28 numbers. The program marks them and won't touch a pin until all are right. Then you blink lab 0's LED and read a button with no `gpio_…()` calls at all.
- **C3** shows how the SDK wraps registers in structs, proves in the disassembly that it costs nothing, and makes a program hang by removing one `volatile`. It ends by answering questions from labs 4 and 5 straight from the datasheet.

Do C1 and C2 now. C3 is worth doing before lab 6, because PIO code is mostly `pio_hw->…` structs.

| # | Lab | Status |
|---|---|---|
| C1 | Bits, masks and register fields | ⬜ next |
| C2 | GPIO from raw registers | ⬜ |
| C3 | Register structs, `volatile` and the disassembly (before lab 6) | ⬜ |

"Lab N" without a C (lab 4's toolbox, lab 0's LED …) means a lab in the [main labbook](labbook.md). Same rules as there: replace the `TODO`s yourself, use the hints only when stuck, fill in **Notes**, and a lab is done when every **Checks** box is ticked.

---

## Reading the datasheet

The file is `docs/RP-008371-DS-1-rp2040-datasheet.pdf` (build date 2025-02-20). All section, table and page numbers in this book are from that version. If you download a newer one, the section numbers usually stay the same, but table and page numbers can move.

**Page numbers.** "Page 248" here means the number printed at the bottom of the page. Your PDF viewer counts the cover too, so it shows one more (249). Go by the printed number, or just search.

**How it's organised.** Chapter 1 is an overview. **Chapter 2, "System Description"**, is everything that isn't an I/O peripheral: bus fabric, address map, SIO, the CPU cores, resets, clocks, GPIO and pads. Chapter 4 is the peripherals (UART, PWM, ADC …), and Appendix B is the **errata**: known chip bugs. Each block follows the same pattern:

```text
2.19. GPIO                       ← the block
  2.19.1. Overview               ← what it does, in prose. Read this first.
  2.19.2. Function Select        ← the details, often with SDK code excerpts
  …
  2.19.6. List of Registers      ← the reference part
     "The … registers start at a base address of 0x… (defined as …_BASE in SDK)"
     Table: List of registers    ← Offset | Name | Info, one line per register
     then one table per register ← Bits | Description | Type | Reset
```

**Reading a register table.** Here's a made-up register in the datasheet's format:

```text
EXAMPLE: CTRL Register
Offset: 0x10
Bits    Description                        Type   Reset
31:16   Reserved.                          -      -
15:8    DIV: clock divider                 RW     0x01
7:3     Reserved.                          -      -
2:1     MODE: 0x0 → off, 0x1 → slow, …     RW     0x0
0       EN: enable                         RW     0x0
```

- **Bits** `15:8` is a field 8 bits wide, from bit 15 down to bit 8. In C1's terms: `lsb` = 8, `width` = 15 − 8 + 1 = 8. A single number is a 1-bit field.
- **Type:** **RW** read/write, **RO** read-only, **WO** write-only, **SC** self-clearing (hardware sets it back to 0), **WC** write 1 to clear (lab 4's DMA flags).
- **Reset** is the value right after power-up. It's how you predict what a register holds before your code has touched it. `-` means it isn't fixed: it depends on the chip or on other hardware.
- **Reserved** bits: write them as 0, or leave them as you read them. Never put your own data there.
- **Registers that come one per pin or per channel** are described only once, with a name like `GPIO0_CTRL, GPIO1_CTRL, …, GPIO29_CTRL` and a list of offsets. Search for `GPIO0_CTRL`, not `GPIO29_CTRL`.

**The SDK uses the datasheet's names.** The SDK's `hardware/regs/*.h` files are generated from the same register descriptions as the datasheet's tables. The pattern is `BLOCK_REGISTER_FIELD_…`:

```text
datasheet: PADS_BANK0, register GPIO0, field PUE, bit 3
SDK:       PADS_BANK0_GPIO0_PUE_BITS  = 0x00000008   PADS_BANK0_GPIO0_PUE_LSB = 3
```
So if you know a name from the datasheet, you can search the SDK for it, and the other way round. C2 uses that to mark your answers.

**Searching.** Search the PDF (Cmd-F) for:
- **the register's name**: `GPIO_OUT_XOR`, `CHIP_ID`, `GPIO0_CTRL`
- **`List of Registers`**, which lands at the reference part of each block
- **the SDK's name for a block's base address**: `IO_BANK0_BASE`

Most hits are in the table of contents or in SDK code excerpts. Skip those.

**Worked example: lab 4's PWM `CC` register.** In lab 4 the DMA wrote to `&pwm_hw->slice[slice_num].cc`. Where does that land?
1. Search `List of PWM`. You get **Section 4.5.3**, page 529: "The PWM registers start at a base address of 0x40050000", then **Table 516**, the list of PWM registers.
2. The list goes `0x00 CH0_CSR`, `0x04 CH0_DIV`, `0x08 CH0_CTR`, `0x0c CH0_CC`, `0x10 CH0_TOP`, then `0x14 CH1_CSR`. So each slice has 5 registers, and **one slice is 0x14 bytes**. CC is at **0x0c** inside each slice.
3. Slice 7 (GP14 and GP15, from labs 1–4): CC is at `0x40050000 + 0x14 × 7 + 0x0c` = **`0x40050098`**.
4. Then the register's own table (**Table 520**, page 532) shows CC's two fields: A in bits 15:0, B in 31:16. That's the same split lab 4's background described.

That's the whole method: **block → base address → list of registers → offset → register table → fields.** C2 has you do it 28 times.

---

## Lab C1 — Bits, masks and register fields

### Goal
Write the six helpers every register program is built from (set, clear, toggle and test a bit, read and write a multi-bit field), and get all the checks to PASS. Then look up two real registers in the datasheet, the CPU's ID and the chip's ID, and decode them with those helpers.

### Background
- **What a register is.** A register is a 32-bit number that lives at a fixed address. Some of its bits are on/off switches, and some groups of bits form small numbers, called **fields**. In the [made-up CTRL register above](#reading-the-datasheet), `EN` is a switch, while `MODE` and `DIV` are fields. Almost all register code does one of four things: read a field, write a field, set or clear a single bit, or wait for a bit to change.

- **If you know Python:** `& | ^ ~ << >>` are the same operators with the same meaning. Three things differ, and all three bite:
  - **Python ints have no width and can be negative. A `uint32_t` is exactly 32 bits and never negative.** `~0x56` in Python is `-87`. In C with an unsigned 32-bit value it's `0xFFFFFFA9`: every one of the 32 bits flipped. `0u - 1` wraps round to `0xFFFFFFFF` (lab 3's phase did this on purpose).
  - **Shifting by 32 or more is undefined in C.** `1 << 40` is fine in Python. In C, `1u << 32` can give anything (lab 4's toolbox).
  - **Precedence: `&` binds *weaker* than `==` in C.** `if (x & MASK == 0)` means `x & (MASK == 0)`, which is nonsense. Python has it the other way round. In C, always bracket bit operations: `if ((x & MASK) == 0)`.

- **The four single-bit operations.** All of them start from the same mask, `1u << k`: a 1 at position *k* and 0 everywhere else.

  ```text
  set    x | (1u << k)       OR with 1 forces a 1; OR with 0 leaves the bit alone
  clear  x & ~(1u << k)      ~ turns the mask into "all 1s except bit k"; AND with that zeroes bit k only
  toggle x ^ (1u << k)       XOR with 1 flips; XOR with 0 leaves alone
  test   (x >> k) & 1u       shift bit k down to bit 0, then throw away the rest → 0 or 1
  ```

- **Fields: mask, then shift.** A datasheet field `hi:lo` starts at `lsb` = lo and is `width` = hi − lo + 1 bits wide:

  ```text
  (1u << width) - 1             width = 12  →  0000 0000 0000 0000 0000 1111 1111 1111   twelve 1s
  … << lsb                      lsb = 4     →  0000 0000 0000 0000 1111 1111 1111 0000   = 0x0000FFF0
  get:  (x & mask) >> lsb       keep only the field, then slide it down to bit 0
  set:  (x & ~mask) | ((value << lsb) & mask)
                                 └ make a hole ┘   └ slide the new value up, cut it to fit ┘
  ```
  The final `& mask` in `set` matters. Without it, a value too big for the field spills into the neighbouring field, which in a real register means changing a setting you never meant to touch. One of the checks tests exactly that.

- **The SDK spells fields the same way.** Every field in `hardware/regs/*.h` has a `_BITS` (the mask, already shifted) and an `_LSB`:

  ```c
  part = (chip_id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
  ```
  That's your `field_get()` with the mask pre-computed. Once you can read this line, you can read most of the SDK.

- **Reading a register: a pointer to a fixed address.** In C you can turn a number into an address and read what's there:

  ```c
  volatile uint32_t *p = (volatile uint32_t *)CPUID_ADDR;   // "p is the address of a 32-bit word"
  uint32_t cpuid = *p;                                       // "the word at that address"
  ```
  The cast `(volatile uint32_t *)` says "treat this number as the address of a `uint32_t`". The `*` in front of `p` **dereferences** it: it means the thing at the address, not the address itself. In Python, the nearest thing is `ctypes.c_uint32.from_address(addr).value`. `volatile` tells the compiler "this memory can change by itself, so really read it every time". That matters for hardware, and C3 shows what happens without it.

- **A wrong address can crash the chip.** Reading an address where nothing is mapped causes a **bus fault**, and the program stops dead. That's why C1 and C2 check your datasheet numbers first and refuse to read anything until they're right.

- **Two different ID registers.** CPUID is inside the ARM Cortex-M0+ core, so it's the same on every chip built with an M0+. The datasheet lists it with the core's own registers (look in the Cortex-M0+ section of chapter 2). CHIP_ID belongs to Raspberry Pi's own SYSINFO block. It holds the maker's JEDEC code, the part number and the silicon revision.

### C toolbox
New C in this lab. For fixed-width types, shifts, hex literals and the bitwise operators, see the toolboxes of labs 4 and 5.

- **Functions that return a new value.** Each helper takes `x` and returns a changed copy. C passes numbers **by value**: the function gets its own copy, so `bit_set(x, 3)` can't change your `x` (like a Python int). To change it, you assign: `x = bit_set(x, 3);`.

- **`bool`.** `true` / `false`, from `<stdbool.h>` (pulled in by `pico/stdlib.h`). When you return a number from a `bool` function, anything non-zero becomes `true`.

- **`putchar(c)`** prints one character. `'1'` in single quotes is a single `char`, while `"1"` in double quotes is a string. `%c` in `printf` prints a number as the character it stands for: `0x41` is `'A'`.

- **`k % 4`** is the remainder after dividing by 4, the same as Python's `%` for positive numbers. `print_bin()` uses it for the spaces between groups of 4.

- **`int k` for a loop that counts down.** `for (int k = 31; k >= 0; k--)` must use a signed `int`. With `uint32_t k`, the condition `k >= 0` is always true: after 0 comes 4 294 967 295, not −1. It's the same trap as lab 5's `uint8_t` loop.

### Parts
Nothing. Just the board on USB, and the datasheet.

### Tasks

**C1.1 Build the untouched skeleton.** Pick `c1-bits/labc1_bits` when you click Run. It builds with zero warnings. It runs, but the warm-up prints nothing, almost every check FAILs, and Part B refuses to read any register.

**C1.2 Warm-up.** TODO 1: `print_bin()`. Before you look at the output, write your predictions for all 8 warm-up lines in Notes. Which two would Python print differently?

**C1.3 The helpers.** TODOs 2–8. Do them one at a time and re-run after each: watch FAILs turn into PASSes. Stop when it prints `0 failed`.

**C1.4 Datasheet: find the two registers** (D1)
- Read the [Reading the datasheet](#reading-the-datasheet) section first.
- **CPUID:** find the list of the Cortex-M0+ registers. What's the block's base address, and what's CPUID's offset? Careful: the offsets in that list are large. Add them up and put the result in `CPUID_ADDR`.
- **CHIP_ID:** find the SYSINFO block. Base address + offset → `CHIP_ID_ADDR`.
- Write the table number and page next to each `#define`, then run: both `D1` checks should PASS.

**C1.5 Datasheet: decode them** (TODOs 9–11)
- TODO 9: read CPUID.
- TODO 10: find **CPUID's own register table**. Copy its five fields into Notes (bits, name, reset value), then decode with `field_get()`. Every field you print should match the table's **Reset** column. If one doesn't, your bit range is off.
- TODO 11: find **CHIP_ID's table** and decode it with the SDK's `_BITS` / `_LSB` names.
- Questions for Notes: Why is CHIP_ID's Reset column `-` while CPUID's has values? What does `RO` mean, and what do you think happens if you write to it? The SYSINFO list has a second register, `PLATFORM`. What should it read on a real chip, and why would anyone need it?

### Hints (read only if stuck)
<details><summary>TODO 1: print_bin</summary>

```c
putchar((x >> k) & 1u ? '1' : '0');
if (k % 4 == 0 && k != 0) {
    putchar(' ');
}
```
Delete the `(void)x;` line once `x` is used. The `?:` is lab 5's conditional operator.
</details>

<details><summary>Warm-up answers</summary>

```text
x       = 0000 0000 0000 0000 0000 0000 0101 0110   0x56
~x      = 1111 1111 1111 1111 1111 1111 1010 1001   0xFFFFFFA9   (Python: -87)
x << 4  = 0000 0000 0000 0000 0000 0101 0110 0000   0x560
x >> 4  = 0000 0000 0000 0000 0000 0000 0000 0101   0x5
x & 0xF = 0000 0000 0000 0000 0000 0000 0000 0110   0x6
x | 1   = 0000 0000 0000 0000 0000 0000 0101 0111   0x57
x ^ x   = 0000 0000 0000 0000 0000 0000 0000 0000   anything XOR itself is 0
0u - 1  = 1111 1111 1111 1111 1111 1111 1111 1111   0xFFFFFFFF   (Python: -1)
```
</details>

<details><summary>TODOs 2–5: single bits</summary>

```c
return x | (1u << k);       // set
return x & ~(1u << k);      // clear
return x ^ (1u << k);       // toggle
return (x >> k) & 1u;       // test
```
Delete the `(void)` lines and the placeholder `return` each time.
</details>

<details><summary>TODOs 6–8: fields</summary>

```c
return ((1u << width) - 1u) << lsb;                      // field_mask

return (x & field_mask(lsb, width)) >> lsb;              // field_get

uint32_t mask = field_mask(lsb, width);                  // field_set
return (x & ~mask) | ((value << lsb) & mask);
```
`field_mask()` can't make a 32-bit-wide field, because `1u << 32` is undefined. That's why its comment says width 1 … 31.
</details>

<details><summary>D1: where to look</summary>

- CPUID: **Section 2.4.8 "List of Registers"** of the Cortex-M0+ section, **Table 87**, page 77. Its first line gives the base address. CPUID is the row `0xed00`.
- CHIP_ID: **Section 2.20 "Sysinfo"**, **Table 349**, page 305. Its first line gives the base address. CHIP_ID is the first row.
</details>

<details><summary>TODOs 9–11: CPUID and CHIP_ID</summary>

CPUID's fields are in **Table 104**, page 84. CHIP_ID's are in **Table 350**, page 305.

```c
volatile uint32_t *p = (volatile uint32_t *)CPUID_ADDR;
uint32_t cpuid = *p;
...
uint32_t impl = field_get(cpuid, 24, 8);
printf("implementer 0x%02lx '%c', variant %lu, architecture 0x%lx, partno 0x%03lx, revision %lu\n",
       impl, (char)impl, field_get(cpuid, 20, 4), field_get(cpuid, 16, 4),
       field_get(cpuid, 4, 12), field_get(cpuid, 0, 4));

uint32_t chip_id = *(volatile uint32_t *)CHIP_ID_ADDR;     // cast and dereference in one go
uint32_t part = (chip_id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
```
`%02lx` prints hex padded to at least 2 digits.

CHIP_ID has no fixed reset value because it's different on each silicon revision: the REVISION field tells them apart. PLATFORM (Table 351) says whether the code is running on the real chip (ASIC) or on an FPGA model of it, which Raspberry Pi used before the chip existed.
</details>

### Stretch challenges
- [ ] **Count the ones:** write `count_bits(x)`, the number of 1 bits in x, with a loop over all 32 bits. Then look up the trick `x & (x - 1)`: what does it do to the lowest 1 bit?
- [ ] **Is it a power of two?** Write it in one line using the same trick. `TABLE_SIZE` and `DAC_CODES` in labs 3 and 5 had to be powers of two. Why?
- [ ] **Read PLATFORM too:** add its address (same block as CHIP_ID) and decode its two bits.
- [ ] **Compare with the SDK:** `rp2040_chip_version()` (declared in `pico/platform.h`, written in the SDK's `rp2040/pico_platform/platform.c`) decodes the same REVISION field. Read how it does it, then print it next to yours.

### Checks
- [ ] Warm-up predictions written down before running, and differences understood
- [ ] All Part A checks PASS (`0 failed`)
- [ ] Both D1 checks PASS, with table numbers written next to the `#define`s
- [ ] CPUID decodes to implementer `0x41` (`'A'` for ARM) and partno `0xC60` (Cortex-M0+): every field matches the Reset column
- [ ] CHIP_ID decodes to manufacturer `0x927` (Raspberry Pi's JEDEC code) and part `0x2`
- [ ] Explain why `field_set()` needs the final `& mask`
- [ ] Explain what `if (x & MASK == 0)` really does
- [ ] Zero warnings

### Notes
*Date:*

*Warm-up predictions vs output:*

*Where I found CPUID and CHIP_ID (section / table / page):*

*CPUID's fields from the datasheet, and what I read:*

| Bits | Field | Reset (datasheet) | My value |
|---|---|---|---|
| | | | |

*CHIP_ID raw value and fields:*

*Answers: CHIP_ID's Reset `-`, RO, PLATFORM:*

*Questions for next time:*

---

## Lab C2 — GPIO from raw registers

### Goal
Find every address, offset and bit number for a GPIO pin in the datasheet: 28 numbers in all. Then blink lab 0's LED and read a button using only those numbers and your C1 bit operations: no `gpio_init()`, `gpio_put()` or `gpio_get()`. Afterwards you'll know what those functions really do, and how to find any register on the chip yourself.

### Background
- **A GPIO pin is controlled by three register blocks.** `gpio_init()` + `gpio_set_dir()` + `gpio_put()` from lab 0 hide all three:

  ```text
     IO_BANK0                         PADS_BANK0                      SIO
     "who drives the pin?"            "the electrical side"           "the pin levels"
     GPIOn_CTRL.FUNCSEL:              pull-up / pull-down,            GPIO_OUT, GPIO_OE, GPIO_IN:
     SIO, PWM, UART, SPI, PIO …       input enable, drive strength    one bit per pin, all 30 pins
  ```
  - **FUNCSEL** picks which peripheral owns the pin. The datasheet's **GPIO function table** has one row per pin and one column per function number, F1 to F9. Lab 2's `gpio_set_function(14, GPIO_FUNC_PWM)` wrote 4 into GP14's FUNCSEL, because in GP14's row, `PWM7 A` sits in column **F4**. You'll find SIO's column the same way. The CTRL register's own table tells you what FUNCSEL holds at reset, and what that means.
  - **SIO** (lab 5) holds the pins' actual values: **GPIO_OE** says which pins are outputs (1) or inputs (0), **GPIO_OUT** is the level each output drives, and **GPIO_IN** reads the level on every pin.

- **Addresses: base + first offset + stride × n.** Each block starts at a **base** address. Registers that come one per pin are spaced evenly: the datasheet gives the offset of pin 0's register, and you work out the **stride** (the distance from one pin's register to the next) from the list.

  ```text
  GPIO_CTRL(n) = ADDR_IO_BANK0   + CTRL_FIRST + CTRL_STRIDE × n
  PAD(n)       = ADDR_PADS_BANK0 + PAD_FIRST  + PAD_STRIDE  × n
  ```
  It's the same calculation as the PWM `CC` example in [Reading the datasheet](#reading-the-datasheet). Look at the list of IO_BANK0 registers to see why the CTRL stride isn't 4: something sits between one pin's CTRL and the next.

- **The pad register.** One register per pin, holding the pin's electrical settings: output disable (OD), input enable (IE), drive strength, pull-up (PUE), pull-down (PDE), Schmitt trigger and slew rate. Its **Reset** column tells you how every pin starts. Decoding it is task C2.1's last step. The answer tells you whether the button needs TODO 5.

- **Read-modify-write.** To change some bits and keep the others, you read the register, change the bits you want, and write it back:

  ```c
  REG(a) = (REG(a) & ~FUNCSEL_MASK) | FUNCSEL_SIO;     // C1's field_set, in one line
  ```
  That's **three** steps: load, modify, store. If an interrupt runs between the load and the store and changes the same register, your store overwrites its change. In this lab nothing else touches these registers, so it's safe. In lab 8, USB interrupts will be running all the time.

- **Atomic aliases: the hardware does the modify.** Most peripheral registers on the RP2040 appear at **four** addresses: the normal one, and three more where writing a mask XORs, sets or clears just those bits. Section 2.1.2 gives the offsets. A write to an alias is **one** store, so nothing can sneak in between. Lab 4's write-1-to-clear DMA flags solve the same problem in a different way.

- **SIO is different.** It's wired straight to the CPU cores, not to the main bus, so it has no aliases. Instead some of its registers come with their own SET, CLR and XOR versions: `GPIO_OUT_SET`, `GPIO_OUT_XOR` and so on. Lab 5's `gpio_put_masked()` wrote to `gpio_togl`, which is `GPIO_OUT_XOR`. That's why you'll write `1u << 29` to a SET register, not `GPIO_OUT |= 1u << 29`.

- **Resets.** Section 2.14: at power-up, most blocks are **held in reset** and do nothing until software releases them in the RESETS block. Before `main()` runs, the SDK's startup code releases them for you, IO_BANK0 and PADS_BANK0 included. On a truly bare-metal chip you'd write the RESETS registers before anything else. TODO 8 lets you see what the SDK did.

### C toolbox
New C in this lab. For pointers, casts and `volatile`, see lab C1.

- **`#define REG(addr) (*(volatile uint32_t *)(addr))`** is a **macro with a parameter**. The preprocessor swaps the text before compiling: `REG(SIO_GPIO_XOR) = m;` becomes `(*(volatile uint32_t *)((ADDR_SIO + OFS_GPIO_XOR))) = m;`, and then your numbers. It's not a function call: it's the C1 pointer cast and dereference, written once and reused.

- **Why macros are full of brackets.** Macros paste text, they don't calculate. Without the brackets around `n`:

  ```c
  #define GPIO_CTRL(n)  (ADDR_IO_BANK0 + CTRL_FIRST + CTRL_STRIDE * n)
  GPIO_CTRL(28 + 1)     →  … + CTRL_STRIDE * 28 + 1          wrong register: GP28's CTRL + 1 byte
  ```
  With `CTRL_STRIDE * (n)` it becomes `CTRL_STRIDE * (28 + 1)`. The rule: put brackets around every parameter, and around the whole macro.

- **The `u` on addresses.** Addresses like the SIO base are too big for a signed `int`. Writing them with a `u` (`0x…u`) makes every address unsigned from the start, so no sign surprises creep into the address maths.

- **Macros built from macros.** `PAD_PUE` is `(1u << PAD_PUE_BIT)`, so you only type the bit number from the datasheet once. `datasheet_checks()` compares the resulting mask with the SDK's `PADS_BANK0_GPIO0_PUE_BITS`.

- **A `?:` that picks an address.** `REG(pressed ? SIO_GPIO_SET : SIO_GPIO_CLR) = mask;` chooses which register to write. It's legal because `REG(…)` just needs a number.

- **`%-14s`** pads a string to 14 characters, left-aligned, so the `show()` lines form columns.

### Parts
- One jumper wire (male–male): the "button"
- The datasheet

### Wiring (zone A)
- LED on GP29 from lab 0 (330 Ω → LED → GND). Nothing to change.
- Jumper wire from **GP28** to a free row. Touching its other end to the **GND rail** is "pressing the button". If GP28 still goes to anything from earlier labs, unplug it.

### Tasks

**C2.1 Datasheet hunt** (D1–D8, in the `#define`s at the top of `main.c`)
- Build and run the untouched skeleton (`c2-raw-gpio/labc2_raw_gpio`, zero warnings). It prints `Datasheet checks` with 28 FAILs, then `No pin touched`. Every FAIL shows what you have, but **not** the right answer.
- Find each number using [the method above](#reading-the-datasheet): block → base address → list of registers → offset → register table → fields. Each `#define` comment says which section or table to look for. Write the table number next to every value as you go, and fill in the worksheet in Notes.
- Rebuild and run after each group. Done when it prints `0 wrong`.
- **Then predict, from the Reset columns:**
  - what GP29's CTRL register holds before your code touches it, and what that FUNCSEL value means
  - what GP29's pad register holds. Decode it field by field in Notes: is the pull-up or the pull-down on? Is input enabled?
  - what GPIO_OE holds

**C2.2 Who took the GPIO out of reset?** TODO 0: paste your C1 `print_bin()`. TODO 8: print whether IO_BANK0 and PADS_BANK0 are in reset.
- Compare RESET with Table 202's Reset column. What did every bit start as, and what is it now?
- RESET_DONE (Table 204) shows which blocks have *finished* coming out of reset. How many bits are set? Does that match the number of blocks in Table 202?
- Find where the SDK does it: open the SDK's `rp2_common/pico_runtime_init/runtime_init.c` and read `runtime_init_early_resets()` and the two `unreset_…` calls after it. Why are ADC, UART, SPI and USB released only *after* the clocks are set up?

**C2.3 Part A: blink** (`PART_B 0`)
- TODO 1: FUNCSEL. TODO 2: output enable. TODO 3: the XOR blink.
- Compare the "before" and "after" lines with your predictions. Which bit of GPIO_OE is now 1? Why that one?
- Watch `GPIO_OUT` while it blinks: which bit changes?
- Comment out TODO 1 only. Does the LED still blink? Does `GPIO_OUT` still change? Why? Put it back.

**C2.4 Part B: button** (`PART_B 1`)
- TODO 4: FUNCSEL for GP28. TODO 5: the pull-up with read-modify-write. TODO 6: read `GPIO_IN`.
- Check the pad value before and after TODO 5. Is it what you calculated from the pad table? Touch the wire to GND: the LED comes on and it prints `pressed`.
- Skip TODO 5 (comment it out). What does the button do now, and why? Your pad decode from C2.1 should predict it. Put it back.
- **Is TODO 4 needed to *read* a pin?** Find the GPIO function descriptions table (the one right after the function table) and read the SIO row. Then comment out TODO 4 and test it. Put it back.

**C2.5 Atomic aliases** (`USE_ALIASES 1`). TODO 7. Check the pad still reads the same as with TODO 5. Then explain in Notes why the alias version would be safe if an interrupt also changed this pad register, and the read-modify-write version wouldn't. Section 2.1.2 says it in one sentence: copy that sentence.

### Hints (read only if stuck)
<details><summary>D1–D8: where to look</summary>

| D | What | Where | Page |
|---|---|---|---|
| D1 | Base addresses | Section 2.2.2 "Detail" (under Address Map), the APB and IOPORT lists | 24–26 |
| D2 | GPIO0_CTRL offset, stride | Section 2.19.6.1, **Table 283** "List of IO_BANK0 registers" | 244 |
| D3 | FUNCSEL mask | **Table 285** "GPIO0_CTRL, …, GPIO29_CTRL", the FUNCSEL row | 248 |
| D3 | SIO's function number | Section 2.19.2, **Table 279** "GPIO User Bank Functions" | 237 |
| D4 | GPIO0 pad offset, stride | Section 2.19.6.3, **Table 339** "List of PADS_BANK0 registers" | 300 |
| D5 | PDE, PUE, IE, OD bits | **Table 341** "GPIO0, …, GPIO29 Registers" | 301 |
| D6 | SIO offsets | Section 2.3.1.7, **Table 16** "List of SIO registers" | 42 |
| D7 | Alias offsets | Section 2.1.2 "Atomic Register Access" | 17 |
| D8 | RESET / RESET_DONE offsets | Section 2.14, **Table 201** "List of RESETS registers" | 175 |
| D8 | IO_BANK0 / PADS_BANK0 bits | **Table 202** "RESET Register" | 175 |

- D2: the stride isn't 4, because IO_BANK0 has a STATUS register before each CTRL. Take the offset of GPIO1_CTRL minus GPIO0_CTRL.
- D3: FUNCSEL is bits 4:0. As a mask, that's five 1s at the bottom. C1's `field_mask(0, 5)` would give it to you.
- D5: give the bit number, not the mask: for a field in bit 3, write `3`.
</details>

<details><summary>"Before" predictions and the pad decode</summary>

- GP29 CTRL: `0x0000001f`. FUNCSEL 31 means `NULL`: the pin is connected to nothing.
- GPIO_OE: `0x00000000`, so there are no outputs yet. After TODOs 1–2, CTRL reads `0x00000005` and GPIO_OE reads `0x20000000`, which is bit 29.
- GP29 pad: `0x00000056`, from Table 341's Reset column:

```text
0x56 = 0101 0110
bit    7    6    5‥4      3     2     1        0
       OD   IE   DRIVE    PUE   PDE   SCHMITT  SLEWFAST
       0    1    01       0     1     1        0
       │    │    4 mA     │     └ pull-down ON
       │    └ input enabled (GPIO_IN works)
       └ output allowed
```
So every pin starts with a **pull-down**. A button to GND needs pull-up on and pull-down off: `0101 1010` = `0x5A`.
</details>

<details><summary>TODO 8 and the resets</summary>

```c
uint32_t r = REG(ADDR_RESETS + OFS_RESET);
printf("IO_BANK0 %s, PADS_BANK0 %s\n",
       (r >> RESET_IO_BANK0_BIT) & 1u ? "in reset" : "running",
       (r >> RESET_PADS_BANK0_BIT) & 1u ? "in reset" : "running");
```
- Table 202's Reset column is `0x1` for every one of the 25 blocks (bits 24:0): everything starts in reset. By `main()`, RESET reads `0x00000000` and RESET_DONE has all 25 low bits set (`0x01ffffff`). The SDK has released every block.
- `runtime_init.c` does it in steps. First it resets nearly everything, so the chip is in a known state even after a soft reboot. It leaves alone the flash pins, the PLLs and USB, because resetting those would break a running system. Then it releases the blocks that only need the system clock, and finally, once the clocks are configured, it releases everything else. The ADC, UARTs, SPI and USB run from their own clocks (clk_adc, clk_peri, clk_usb), so they wait until those clocks exist.
</details>

<details><summary>TODOs 1–3</summary>

```c
REG(GPIO_CTRL(LED_PIN)) = (REG(GPIO_CTRL(LED_PIN)) & ~FUNCSEL_MASK) | FUNCSEL_SIO;
REG(SIO_GPIO_OE_SET) = 1u << LED_PIN;
...
REG(SIO_GPIO_XOR) = 1u << LED_PIN;
```
Without TODO 1, SIO still changes `GPIO_OUT`, but FUNCSEL connects the pin to nothing, so the LED stays dark. SIO only *offers* a value. IO_BANK0 decides whether that value reaches the pin.
</details>

<details><summary>TODOs 4–6, and whether TODO 4 is needed</summary>

```c
REG(GPIO_CTRL(BUTTON_PIN)) = (REG(GPIO_CTRL(BUTTON_PIN)) & ~FUNCSEL_MASK) | FUNCSEL_SIO;

REG(PAD(BUTTON_PIN)) = (REG(PAD(BUTTON_PIN)) | PAD_PUE) & ~PAD_PDE;

bool pressed = !((REG(SIO_GPIO_IN) >> BUTTON_PIN) & 1u);
REG(pressed ? SIO_GPIO_SET : SIO_GPIO_CLR) = 1u << LED_PIN;
if (pressed != was_pressed) {
    printf(pressed ? "pressed\n" : "released\n");
    was_pressed = pressed;
}
```
- Without the pull-up, the pull-down holds the pin at 0 all the time, and touching GND changes nothing. It reads "pressed" forever.
- **Table 280**, page 238, SIO row: "The SIO function (F5) must be selected for the processors to drive a GPIO, but the input is always connected, so software can check the state of GPIOs at any time." So TODO 4 is only needed for *outputs*. Reading works whatever FUNCSEL says, as long as the pad's IE bit is on.
</details>

<details><summary>TODO 7: aliases</summary>

```c
REG(PAD(BUTTON_PIN) + ALIAS_SET) = PAD_PUE;
REG(PAD(BUTTON_PIN) + ALIAS_CLR) = PAD_PDE;
```
No read at all. Each line is one store, and the hardware changes only the bits that are 1 in the value you write.
</details>

### Stretch challenges
- [ ] **Errata:** open **Appendix B, "Errata"** and read **RP2040-E6** (page 630). Which of *your* pins does it affect? Does it matter for an LED and a button? Then read **RP2040-E11** on the same page: it's the ADC problem behind lab 5's "is it my ladder or my ADC?".
- [ ] **Drive strength:** decode the pad's DRIVE field with your C1 `field_get()`. What are the four values, according to Table 341?
- [ ] **Input enable:** in Part B, clear `PAD_IE` on GP28 with the CLR alias. What does `GPIO_IN` read now, pressed or not?
- [ ] **Open drain:** make GP29 "low or let go" instead of "low or high": keep `GPIO_OUT` at 0 and toggle `GPIO_OE` (you'll need `GPIO_OE_CLR` from Table 16). Does the LED still light? (It's wired to GND, so think about what drives it high.) I2C works this way.
- [ ] **Read the SDK:** Section 2.19.2 shows an excerpt of `gpio_set_function()`. Find the whole function in the SDK's `hardware_gpio/gpio.c`. Which of your lines matches which of theirs? Do they use aliases?

### Checks
- [ ] All 28 datasheet checks PASS, every value has its table number next to it, and the worksheet is filled in
- [ ] "Before" values predicted from the Reset columns, then confirmed; GP29's pad value decoded field by field
- [ ] RESET / RESET_DONE read and compared with Table 202; I found where in the SDK the blocks are released, and why some wait for the clocks
- [ ] LED blinks with only register writes
- [ ] Explain why the LED stays dark without TODO 1 even though `GPIO_OUT` changes
- [ ] Button lights the LED. I can say from the datasheet whether TODO 4 is needed, and tested it
- [ ] Explain read-modify-write vs alias writes, and why SIO has its own SET/CLR registers
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:*

*Datasheet worksheet:*

| D | Name | Value | Table | Page |
|---|---|---|---|---|
| D1 | IO_BANK0 base | | | |
| D1 | PADS_BANK0 base | | | |
| D1 | SIO base | | | |
| D1 | RESETS base | | | |
| D2 | GPIO0_CTRL offset / stride | | | |
| D3 | FUNCSEL mask / SIO number | | | |
| D4 | GPIO0 pad offset / stride | | | |
| D5 | PDE / PUE / IE / OD bits | | | |
| D6 | SIO offsets | | | |
| D7 | XOR / SET / CLR aliases | | | |
| D8 | RESET / RESET_DONE / bits | | | |

*Predicted vs measured "before" / "after" values, and my pad decode:*

*RESET / RESET_DONE values, and what the SDK's runtime_init.c does:*

*Without TODO 1:*

*Without TODO 4 / without TODO 5:*

*Why aliases are safe (with the sentence from 2.1.2):*

*Questions for next time:*

---

## Lab C3 — Register structs, `volatile` and the disassembly

### Goal
Turn the datasheet's list of SIO registers into your own C struct, the way the SDK does it. Toggle the LED four ways (raw address, your struct, the SDK's `sio_hw`, `gpio_xor_mask()`) and check in the `.dis` file that they all compile down to the same single store. Then remove `volatile` from a button loop and find the exact instruction that makes the program hang. Finally, answer four questions from labs 4 and 5 with the datasheet.

### Background
- **Struct overlays.** C2 needed one `#define` per register. A struct describes a whole block at once: the fields are laid out one after another in memory, 4 bytes each, so a field's offset in the struct *is* the register's offset in the block. The struct is a copy of the datasheet's list of registers, written in C:

  ```text
  Table 16 (List of SIO registers)        my_sio_t at 0xD0000000
  Offset  Name                            offset   field
  0x000   CPUID                    →      0x000    volatile uint32_t cpuid;
  0x004   GPIO_IN                  →      0x004    volatile uint32_t gpio_in;
  0x008   GPIO_HI_IN               →      0x008    volatile uint32_t gpio_hi_in;
  …       (read the Offset column carefully from here on)
  ```
  `#define MY_SIO ((my_sio_t *)0xD0000000u)` says "at this address there's a `my_sio_t`". Then `MY_SIO->gpio_togl = m;` writes to the SIO base plus `gpio_togl`'s offset. The SDK's `sio_hw`, `pwm_hw` (lab 4) and `pio_hw` (lab 6) are exactly this. In Python terms it's a `ctypes.Structure` laid over memory with `from_address()`.

- **A gap in the list must become a gap in the struct.** If the datasheet skips an offset, the struct has to skip it too, with a dummy field. Otherwise every field after it moves 4 bytes and points at the wrong register. The SDK calls these `_pad0`, `_pad1` ….

- **Zero cost.** A struct is only a description for the compiler. In the compiled code there's no struct and no function call left, just "store this number at that address". You'll see it in the disassembly.

- **What `volatile` really changes.** The optimiser assumes memory only changes when *your code* writes to it. So in

  ```c
  while (*in & BUTTON_MASK) {}      // in is a plain uint32_t *, not volatile
  ```
  it reasons: "nothing in the loop writes to `*in`, so it has the same value every time". It reads the pin **once**, and if the button wasn't pressed at that moment, it loops forever without ever looking again. With `volatile`, every `*in` in the source becomes a real read in the program. Use `volatile` for anything that can change behind the code's back: hardware registers, and variables shared with an interrupt (labs 3 and 4).

- **Reading the `.dis` file.** `pico_add_extra_outputs()` writes `build/<lab>/<exe>.dis`, the whole program as ARM Thumb instructions. You only need a handful to follow this lab:

  ```text
  movs r2, #128         r2 = 128                    (only 8-bit numbers fit in one instruction)
  lsls r2, r2, #22      r2 = r2 << 22               128 << 22 = 2⁷ × 2²² = 2²⁹ = 1u << 29 = LED_MASK
  ldr  r3, [pc, #8]     r3 = a 32-bit constant stored after the function (the ".word" line)
  ldr  r3, [r1, #0]     r3 = the word at address r1           ← a READ
  str  r2, [r3, #28]    the word at address r3 + 28 = r2      ← a WRITE (28 = 0x1C)
  tst  r3, r2           set flags from r3 & r2
  bne  / bpl            branch if not zero / branch if bit 31 is 0
  b.n  <address>        jump; if the address is the instruction itself, that's an endless loop
  bx   lr               return
  ```
  `r0`–`r7` are the CPU's own registers (scratch variables inside the CPU), not peripheral registers.

### C toolbox
New C in this lab. For macros see lab C2, and for pointers and `->` see lab 4.

- **Header files and include guards.** `my_sio.h` holds declarations that more than one `.c` file could share. `#include "my_sio.h"` pastes it in. `#ifndef MY_SIO_H` / `#define MY_SIO_H` / `#endif` makes a second include of the same file paste nothing, because the struct may only be defined once. Quotes `"…"` search the lab's folder first, angle brackets `<…>` only search the system and SDK paths.

- **`typedef struct { … } my_sio_t;`** defines a struct and gives it the one-word name `my_sio_t`. Without `typedef` you'd write `struct my_sio` everywhere (lab 1's `struct note`). The SDK's `_t` ending means "this is a type".

- **`volatile` on each field.** Then every access through the struct is a real read or write, without having to remember it at every use. The SDK's `io_rw_32` / `io_ro_32` / `io_wo_32` are `volatile uint32_t` with a name that also tells you read-write, read-only, write-only: the datasheet's **Type** column.

- **`offsetof(type, field)`** (from `<stddef.h>`) is the field's distance in bytes from the start of the struct. **`_Static_assert(condition, "message")`** checks a condition **while compiling**: if it's false, the build fails with your message. Together they turn "I hope the struct matches the datasheet" into "it won't build unless it does".

- **An array of function pointers.** `void (*ways[])(void) = { toggle_raw, … };` reads inside out: `ways` is an array (`[]`) of pointers (`*`) to functions that take nothing and return nothing (`void … (void)`). `ways[i]()` calls the *i*-th one. Lab 4's callbacks were single function pointers.

- **`__attribute__((noipa))`** is a GCC extension, not standard C. It tells the optimiser to leave the function alone: don't inline it into `main()`, don't merge identical functions (three of them *are* identical), don't change how it's called. Without it, the four toggles could vanish into `main()` and you'd have nothing to find by name. VS Code's checker doesn't know `noipa` and may underline it. The real compiler does.

- **`%p`** prints a pointer (an address) in hex. It expects a `void *`, which is "an address of anything", so the code casts with `(void *)`.

### Parts
Same as lab C2.

### Tasks

**C3.1 Build the untouched skeleton.** Pick `c3-reg-structs/labc3_reg_structs`. Zero warnings. Part A runs, but only `raw` blinks. At startup, `&MY_SIO->gpio_hi_in` and `&sio_hw->gpio_hi_in` should print the same address. Which one, and why?

**C3.2 Your struct, from Table 16** (`my_sio.h`)
- Open the datasheet's **list of SIO registers** (Section 2.3.1.7).
- **Test first:** TODO 2 before TODO 1. Write `_Static_assert`s for `gpio_out`, `gpio_togl` and `gpio_oe_clr` with the offsets **from the datasheet**. It won't build yet, because the fields don't exist.
- TODO 1: add the fields, copying the list in order. Build. If an assert fails, compare your struct line by line with the **Offset** column. What did you miss?
- Prove the asserts work: swap `gpio_set` and `gpio_clr`, and build. Read the error. Put it back.
- Read the Type column of the SIO registers' own tables (e.g. **GPIO_OUT_XOR**). Which of your fields would the SDK declare `io_wo_32`, and which `io_ro_32`? Compare with the SDK's `hardware/structs/sio.h`.

**C3.3 Part A: four ways, one instruction** (`PART_B 0`)
- TODOs 3–5. All four ways should now give 2 blinks each.
- Open `build/c3-reg-structs/labc3_reg_structs.dis` and search for `<toggle_raw>:`, `<toggle_my_struct>:`, `<toggle_sdk_struct>:`, `<toggle_sdk_function>:`. Copy them into Notes.
- Which three are byte-for-byte identical? The fourth reaches the same address another way: how? Find `str` in each one and work out the address and the value it writes. Which register in Table 16 is that address?

**C3.4 Part B: the missing `volatile`** (`PART_B 1`, `USE_VOLATILE 1`)
- TODO 6. Touch GP28 to GND: `pressed!` and a flash. Repeat a few times.
- Now `USE_VOLATILE 0`. Predict what happens before you flash it. Then try it, *without* touching GND at startup.
- Find `<wait_for_press>:` in the `.dis` for both versions and copy them into Notes. Point at the instruction the plain version loops on forever. Where is the `ldr` in each version: inside the loop or before it?
- Set `USE_VOLATILE` back to 1.

**C3.5 Datasheet questions from earlier labs.** Find each answer in the datasheet. Write the answer in Notes, with the section and page, and quote the sentence that settles it.
1. **Lab 5:** "the DMA can't reach the SIO". Where does the datasheet say so, and why is the SIO built that way? *(Look at the start of the SIO section.)*
2. **Lab 4:** a 16-bit DMA write to the PWM `CC` register lands in **both** halves, A and B. Which section explains that? Why can't you change only channel B's half with a 16-bit write, and what does the datasheet suggest instead?
3. **C2:** both CPU cores write GPIO_OUT on the same clock cycle, one with SET and one with XOR. What happens?
4. **C2:** atomic aliases. Which peripherals get them from a "bus interposer" instead of natively, and what does that cost?

### Hints (read only if stuck)
<details><summary>TODOs 1–2: the struct</summary>

Table 16 is on page 42. It jumps from `0x008 GPIO_HI_IN` to `0x010 GPIO_OUT`: **there's no register at 0x00C**. That needs a dummy field:

```c
uint32_t _pad0;                 // 0x00C  no register here
volatile uint32_t gpio_out;     // 0x010
volatile uint32_t gpio_set;     // 0x014
volatile uint32_t gpio_clr;     // 0x018
volatile uint32_t gpio_togl;    // 0x01C
volatile uint32_t gpio_oe;      // 0x020
volatile uint32_t gpio_oe_set;  // 0x024
volatile uint32_t gpio_oe_clr;  // 0x028
```
```c
_Static_assert(offsetof(my_sio_t, gpio_out) == 0x010, "gpio_out must be at 0x010");
_Static_assert(offsetof(my_sio_t, gpio_togl) == 0x01C, "gpio_togl must be at 0x01C");
_Static_assert(offsetof(my_sio_t, gpio_oe_clr) == 0x028, "gpio_oe_clr must be at 0x028");
```
Without `_pad0`, `gpio_out` lands at 0x00C and all three asserts fail. Both startup addresses are `0xd0000008` (SIO base + GPIO_HI_IN's offset).

Types: GPIO_IN (Table 18) and GPIO_HI_IN are **RO**, so the SDK declares them `io_ro_32`. GPIO_OUT_XOR (Table 23, page 46) and the other SET/CLR/XOR registers are **WO**, so they're `io_wo_32`. You only write a mask to them, and there's nothing useful to read back. GPIO_OUT and GPIO_OE are **RW**: `io_rw_32`. The SDK's three type names are the datasheet's Type column, written in C.
</details>

<details><summary>TODOs 3–6</summary>

```c
MY_SIO->gpio_togl = LED_MASK;
sio_hw->gpio_togl = LED_MASK;
gpio_xor_mask(LED_MASK);
...
while (*in & BUTTON_MASK) {}
```
Delete `(void)in;`. `gpio_xor_mask()` is a `static inline` function in the SDK's `gpio.h`: a function whose body the compiler copies into the caller, so it costs no call.
</details>

<details><summary>What the disassembly should look like</summary>

The three struct/function versions build `0xD0000000` as `0xd0 << 24`, build `LED_MASK` as `128 << 22`, then `str r2, [r3, #28]`: store at base + 0x1C, which is GPIO_OUT_XOR. `toggle_raw` loads the full address `0xd000001c` from a `.word` after the function and stores at offset `#0`. It's the same write to the same address, just a different route to the number.

`wait_for_press`, plain pointer: one `ldr` **before** the loop, then a test, then `b.n` to its own address. The CPU spins on that one instruction and never reads the pin again. Volatile pointer: the `ldr` is **inside** the loop, followed by `tst` and `bne` back to the `ldr`.
</details>

<details><summary>C3.5: where the answers are</summary>

1. **Section 2.3.1 "SIO"**, the NOTE on page 27: "The SIO is not connected to the main system bus due to its tight timing requirements. It can only be accessed by the processors, or by the debugger via the processor debug ports." The DMA is a master on the main bus, so it can't get there. That's why lab 6 uses PIO, which DMA *can* reach.
2. **Section 2.1.4 "Narrow IO Register Writes"**, page 17: IO registers treat every write as 32 bits, and the Cortex-M0+ and the DMA copy a narrow value across the whole bus. So a 16-bit write of `x` writes `x` into both halves. The datasheet's suggestion is the atomic set/clear aliases, which can change any combination of fields in one write.
3. **Section 2.3.1.2 "GPIO Control"**, page 30: it's as though core 0 wrote first and core 1 immediately afterwards. SET then XOR leaves the bit at 0, whatever it was before.
4. **Section 2.1.2**, page 17: I2C, UART, SPI and SSI get them from a bus interposer, which turns the atomic write into a read-modify-write at the peripheral's boundary. It costs two extra system clock cycles per access.
</details>

### Stretch challenges
- [ ] **Your own `pads_bank0` struct from Table 339:** a `voltage_select` field, then an array `volatile uint32_t gpio[30];`. Redo C2's pull-up as `MY_PADS->gpio[28] = …`. Assert `offsetof(my_pads_t, gpio[28])` against the datasheet's offset for GPIO28.
- [ ] **A struct with an array of structs:** the PWM block (Table 516) is 8 identical slices of 5 registers. Write `my_pwm_slice_t` and `my_pwm_t { my_pwm_slice_t slice[8]; }`, then assert that `&MY_PWM->slice[7].cc` is `0x40050098`, the address from the [worked example](#reading-the-datasheet). That's exactly lab 4's `pwm_hw->slice[slice_num].cc`.
- [ ] **Your own bit helpers as a header:** move the C1 helpers into `bits.h` as `static inline` functions with an include guard, and use it from this lab.
- [ ] **What exactly is volatile?** In the volatile `wait_for_press`, the `ldr r1, [pc, #12]` that loads the *address* stays outside the loop, and only the read *through* it is inside. Why is that allowed? *(Which is volatile: the pointer, or the thing it points to?)* Then look up what `uint32_t *volatile p` would mean.
- [ ] **Optimisation levels:** the lab builds as Release (`-O3`). At `-O0` the plain-pointer loop happens to work, because nothing is optimised; at `-Og` (Debug) it hangs too. Why is "it works in one build mode" not proof that the code is right?

### Checks
- [ ] Struct written from Table 16, asserts written first from the datasheet's offsets, and I saw one fail on purpose
- [ ] Found the gap at 0x00C and explained why the struct needs `_pad0`
- [ ] Four ways blink; disassembly of all four copied, and I found the `str`, its address, and which Table 16 register that is
- [ ] Explain why three are identical and how `toggle_raw` differs
- [ ] Plain-pointer version hangs; I found the endless `b.n` and explained why the `ldr` is outside the loop
- [ ] Explain when a variable needs `volatile`
- [ ] All four C3.5 questions answered with section, page and a quote
- [ ] Zero warnings in the mode you finish in

### Notes
*Date:*

*Addresses printed at startup:*

*What I missed in the struct the first time (if anything):*

*Disassembly of the four toggles:*

*wait_for_press, volatile vs plain:*

*C3.5 answers (section / page / quote):*

1.
2.
3.
4.

*Questions for next time:*
