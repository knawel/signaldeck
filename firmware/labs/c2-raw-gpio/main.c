// Lab C2 — GPIO from raw registers
//
// Goal: find every address and bit number in the datasheet yourself, then blink lab 0's LED and
// read a "button" without any gpio_...() function: only those numbers, a pointer cast, and
// the bit operators from lab C1.
// Hardware: LED on GP29 (zone A, from lab 0). A jumper wire in GP28 is the button: touch it to GND.
// See labs/c-labbook.md, "Lab C2". Datasheet: docs/RP-008371-DS-1-rp2040-datasheet.pdf

#include <stdio.h>
#include "pico/stdlib.h"     // only for printf and sleep_ms: no gpio_...() calls in this lab

// The SDK's own register headers. Used ONLY by datasheet_checks() to mark your numbers,
// never to drive a pin.
#include "hardware/regs/addressmap.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/sio.h"
#include "hardware/regs/resets.h"

// Treat the 32-bit word at address `addr` as a variable: REG(a) = 5;  x = REG(a);
#define REG(addr)       (*(volatile uint32_t *)(addr))

// ===== Datasheet hunt: every 0 below is a number for you to find (TODOs D1–D8). =====
// Write the table number next to each one, e.g.  0x40014000u   // Table 15
// The program checks them all and refuses to touch a pin until every one is right.

// D1: where each register block starts. Section 2.2, "Address Map".
#define ADDR_IO_BANK0   0x40014000u        // chooses which peripheral drives each pin
#define ADDR_PADS_BANK0 0x4001c000u        // the electrical side: pulls, input enable, drive strength
#define ADDR_SIO        0xd0000000u        // single-cycle I/O: the actual pin levels
#define ADDR_RESETS     0x4000c000u        // holds peripherals in reset

// D2: IO_BANK0's list of registers. Offset of GPIO0_CTRL, and the distance from one pin's CTRL to the next.
#define CTRL_FIRST      0x004u
#define CTRL_STRIDE     0x00cu - 0x004u
#define GPIO_CTRL(n)    (ADDR_IO_BANK0 + CTRL_FIRST + CTRL_STRIDE * (n))

// D3: the GPIOn_CTRL register table: FUNCSEL's bits as a mask.
//     Then the GPIO function table: which function number is SIO?
#define FUNCSEL_MASK    0x1fu
#define FUNCSEL_SIO     5u

// D4: PADS_BANK0's list of registers. Offset of the GPIO0 pad register, and the distance to the next one.
#define PAD_FIRST       0x4u
#define PAD_STRIDE      0x4u
#define PAD(n)          (ADDR_PADS_BANK0 + PAD_FIRST + PAD_STRIDE * (n))

// D5: the pad register table: the bit number of each of these fields.
#define PAD_PDE_BIT     2           // pull-down enable
#define PAD_PUE_BIT     3           // pull-up enable
#define PAD_IE_BIT      6           // input enable
#define PAD_OD_BIT      7           // output disable
#define PAD_PDE         (1u << PAD_PDE_BIT)
#define PAD_PUE         (1u << PAD_PUE_BIT)
#define PAD_IE          (1u << PAD_IE_BIT)
#define PAD_OD          (1u << PAD_OD_BIT)

// D6: SIO's list of registers: offsets from ADDR_SIO.
#define OFS_GPIO_IN     0x004u
#define OFS_GPIO_OUT    0x010u
#define OFS_GPIO_SET    0x014u        // the datasheet calls it GPIO_OUT_SET
#define OFS_GPIO_CLR    0x018u        // GPIO_OUT_CLR
#define OFS_GPIO_XOR    0x01cu        // GPIO_OUT_XOR
#define OFS_GPIO_OE     0x020u
#define OFS_GPIO_OE_SET 0x024u
#define SIO_GPIO_IN     (ADDR_SIO + OFS_GPIO_IN)
#define SIO_GPIO_OUT    (ADDR_SIO + OFS_GPIO_OUT)
#define SIO_GPIO_SET    (ADDR_SIO + OFS_GPIO_SET)     // write 1s: those pins go high, others untouched
#define SIO_GPIO_CLR    (ADDR_SIO + OFS_GPIO_CLR)     // write 1s: those pins go low
#define SIO_GPIO_XOR    (ADDR_SIO + OFS_GPIO_XOR)     // write 1s: those pins flip
#define SIO_GPIO_OE     (ADDR_SIO + OFS_GPIO_OE)
#define SIO_GPIO_OE_SET (ADDR_SIO + OFS_GPIO_OE_SET)

// D7: the atomic aliases, added to a register's address. Section 2.1.2. (IO_BANK0, PADS: yes. SIO: no!)
#define ALIAS_XOR       0x1000u
#define ALIAS_SET       0x2000u
#define ALIAS_CLR       0x3000u

// D8: RESETS' list of registers and the RESET register table.
#define OFS_RESET       0x0u        // offset of RESET
#define OFS_RESET_DONE  0x8u        // offset of RESET_DONE
#define RESET_IO_BANK0_BIT   5      // which bit is IO_BANK0
#define RESET_PADS_BANK0_BIT 8      // which bit is PADS_BANK0

// ===== end of the datasheet hunt =====

#define LED_PIN         29
#define BUTTON_PIN      28
#define BLINK_MS        250

// Part A: 0 = blink the LED. Part B: 1 = the LED follows the button.
#define PART_B          1

// 1 = TODO 7: set the pull-up through the SET / CLR aliases instead of read-modify-write
#define USE_ALIASES     0

// Paste your finished print_bin() from lab C1 here (TODO 0).
static void print_bin(uint32_t x) {
    for (int k = 31; k >= 0; k--) {
        bool bit = x >> k & 1;
        putchar(bit ? '1' : '0');
        if ((k % 4 == 0) && (k!=0) ){
            putchar(' ');
        }
    }
}

static int failures;

static void check_ds(const char *what, uint32_t got, uint32_t want) {
    if (got == want) {
        printf("  PASS  %s\n", what);
    } else {
        printf("  FAIL  %s: you have 0x%08lx\n", what, got);
        failures++;
    }
}

// Marks your D-numbers against the SDK's headers, which are generated from the same register
// descriptions as the datasheet. Returns how many are wrong.
static int datasheet_checks(void) {
    failures = 0;
    printf("Datasheet checks\n");
    check_ds("D1 IO_BANK0 base",        ADDR_IO_BANK0,   IO_BANK0_BASE);
    check_ds("D1 PADS_BANK0 base",      ADDR_PADS_BANK0, PADS_BANK0_BASE);
    check_ds("D1 SIO base",             ADDR_SIO,        SIO_BASE);
    check_ds("D1 RESETS base",          ADDR_RESETS,     RESETS_BASE);
    check_ds("D2 GPIO0_CTRL offset",    CTRL_FIRST,      IO_BANK0_GPIO0_CTRL_OFFSET);
    check_ds("D2 CTRL stride",          CTRL_STRIDE,     IO_BANK0_GPIO1_CTRL_OFFSET - IO_BANK0_GPIO0_CTRL_OFFSET);
    check_ds("D3 FUNCSEL mask",         FUNCSEL_MASK,    IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS);
    check_ds("D3 FUNCSEL = SIO",        FUNCSEL_SIO,     IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0);
    check_ds("D4 GPIO0 pad offset",     PAD_FIRST,       PADS_BANK0_GPIO0_OFFSET);
    check_ds("D4 pad stride",           PAD_STRIDE,      PADS_BANK0_GPIO1_OFFSET - PADS_BANK0_GPIO0_OFFSET);
    check_ds("D5 PDE",                  PAD_PDE,         PADS_BANK0_GPIO0_PDE_BITS);
    check_ds("D5 PUE",                  PAD_PUE,         PADS_BANK0_GPIO0_PUE_BITS);
    check_ds("D5 IE",                   PAD_IE,          PADS_BANK0_GPIO0_IE_BITS);
    check_ds("D5 OD",                   PAD_OD,          PADS_BANK0_GPIO0_OD_BITS);
    check_ds("D6 GPIO_IN",              OFS_GPIO_IN,     SIO_GPIO_IN_OFFSET);
    check_ds("D6 GPIO_OUT",             OFS_GPIO_OUT,    SIO_GPIO_OUT_OFFSET);
    check_ds("D6 GPIO_OUT_SET",         OFS_GPIO_SET,    SIO_GPIO_OUT_SET_OFFSET);
    check_ds("D6 GPIO_OUT_CLR",         OFS_GPIO_CLR,    SIO_GPIO_OUT_CLR_OFFSET);
    check_ds("D6 GPIO_OUT_XOR",         OFS_GPIO_XOR,    SIO_GPIO_OUT_XOR_OFFSET);
    check_ds("D6 GPIO_OE",              OFS_GPIO_OE,     SIO_GPIO_OE_OFFSET);
    check_ds("D6 GPIO_OE_SET",          OFS_GPIO_OE_SET, SIO_GPIO_OE_SET_OFFSET);
    check_ds("D7 XOR alias",            ALIAS_XOR,       REG_ALIAS_XOR_BITS);
    check_ds("D7 SET alias",            ALIAS_SET,       REG_ALIAS_SET_BITS);
    check_ds("D7 CLR alias",            ALIAS_CLR,       REG_ALIAS_CLR_BITS);
    check_ds("D8 RESET offset",         OFS_RESET,       RESETS_RESET_OFFSET);
    check_ds("D8 RESET_DONE offset",    OFS_RESET_DONE,  RESETS_RESET_DONE_OFFSET);
    check_ds("D8 IO_BANK0 reset bit",   1u << RESET_IO_BANK0_BIT,   RESETS_RESET_IO_BANK0_BITS);
    check_ds("D8 PADS_BANK0 reset bit", 1u << RESET_PADS_BANK0_BIT, RESETS_RESET_PADS_BANK0_BITS);
    printf("%d wrong\n\n", failures);
    return failures;
}

static void show(const char *name, uint32_t addr) {
    uint32_t value = REG(addr);
    printf("%-14s @ 0x%08lx = 0x%08lx = ", name, addr, value);
    print_bin(value);
    printf("\n");
}

// Task C2.2: who took the GPIO blocks out of reset before main() started?
static void show_resets(void) {
    show("RESET", ADDR_RESETS + OFS_RESET);
    show("RESET_DONE", ADDR_RESETS + OFS_RESET_DONE);
    // TODO 8: test bits RESET_IO_BANK0_BIT and RESET_PADS_BANK0_BIT of REG(ADDR_RESETS + OFS_RESET)
    //         and print for each block whether it is "in reset" (1) or "running" (0).
    uint32_t reset = REG(ADDR_RESETS + OFS_RESET);
    uint32_t io = (reset >> RESET_IO_BANK0_BIT) & 1u;
    uint32_t pads = (reset >> RESET_PADS_BANK0_BIT) & 1u;
    printf("bit=0x%08lx; IO_BANK0:   %s\n", io, io ? "in reset" : "running");
    printf("PADS_BANK0: %s\n", pads ? "in reset" : "running");
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect

    printf("=== Lab C2 ===\n");
    // A wrong address can crash the chip, so nothing below runs until every D-number is right.
    while (datasheet_checks() != 0) {
        printf("No pin touched. Fix the D-numbers, rebuild and flash again.\n\n");
        sleep_ms(5000);
    }

    show_resets();
    printf("before:\n");
    show("GP29 CTRL", GPIO_CTRL(LED_PIN));
    show("GP29 PAD", PAD(LED_PIN));
    show("SIO GPIO_OE", SIO_GPIO_OE);

    // TODO 1: hand GP29 to the SIO. Read-modify-write GPIO_CTRL(LED_PIN):
    //         clear the FUNCSEL bits with & ~FUNCSEL_MASK, then | FUNCSEL_SIO.
    printf("IO_BANK0 BASE: 0x%08X\n", ADDR_IO_BANK0);
    printf("CTRL_FIRST:    0x%08X\n", CTRL_FIRST);
    printf("CTRL_STRIDE:   0x%08X\n", CTRL_STRIDE);
    printf("GPIO_CTRL(29): 0x%08X\n", GPIO_CTRL(29));

    // TODO 2: make GP29 an output: write its bit (1u << LED_PIN) to SIO_GPIO_OE_SET.

    printf("after:\n");
    show("GP29 CTRL", GPIO_CTRL(LED_PIN));
    show("SIO GPIO_OE", SIO_GPIO_OE);

#if !PART_B
    while (true) {
        // TODO 3: flip the LED with one write to SIO_GPIO_XOR.
        show("SIO GPIO_OUT", SIO_GPIO_OUT);
        sleep_ms(BLINK_MS);
    }
#else
    printf("button:\n");
    show("GP28 PAD", PAD(BUTTON_PIN));

    // TODO 4: hand GP28 to the SIO as well (same as TODO 1). Its OE bit stays 0, so it's an input.

#if !USE_ALIASES
    // TODO 5: switch GP28 from pull-down to pull-up with one read-modify-write of PAD(BUTTON_PIN):
    //         set PAD_PUE, clear PAD_PDE.
#else
    // TODO 7: the same, but with two plain writes: PAD_PUE to the SET alias, PAD_PDE to the CLR alias.
#endif
    show("GP28 PAD", PAD(BUTTON_PIN));

    bool was_pressed = false;
    while (true) {
        // TODO 6: read bit BUTTON_PIN of SIO_GPIO_IN. With the pull-up, 1 = released, 0 = touching GND.
        //         LED on (SIO_GPIO_SET) while pressed, off (SIO_GPIO_CLR) while released.
        //         Print "pressed" / "released" only when it changes.
        bool pressed = false;
        (void)was_pressed;
        (void)pressed;
        sleep_ms(10);
    }
#endif
}
