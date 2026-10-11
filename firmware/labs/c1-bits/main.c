// Lab C1 — Bits, masks and register fields
//
// Goal: practise C's bit operators on plain numbers, with PASS/FAIL checks after every run,
// then look up two real registers in the datasheet, read them through a pointer and decode them.
// Hardware: nothing new. See labs/c-labbook.md, "Lab C1". Datasheet: docs/RP-008371-DS-1-rp2040-datasheet.pdf

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/regs/addressmap.h"   // SYSINFO_BASE, PPB_BASE: where register blocks start
#include "hardware/regs/sysinfo.h"      // SYSINFO_CHIP_ID_..._BITS / _LSB
#include "hardware/regs/m0plus.h"       // M0PLUS_CPUID_OFFSET

// D1: find both addresses in the datasheet (base address + offset) and write the table numbers here.
//     The program checks them against the SDK before it reads anything.
#define CPUID_ADDR      (0xE0000000u + 0xED00u)    // M0PLUS block (the Cortex-M0+ core's own registers): CPUID
#define CHIP_ID_ADDR    0x40000000u   // SYSINFO block: CHIP_ID

// --- Part A: bit helpers. Each one takes a number and returns a new one, like a Python function. ---

// Print x as 32 binary digits in groups of 4, e.g. 0000 0000 0000 0000 0000 0000 0101 0110
static void print_bin(uint32_t x) {
    for (int k = 31; k >= 0; k--) {
        // TODO 1: putchar('1') or putchar('0') for bit k of x,
        //         then a space after every 4th digit
        // (when k is a multiple of 4, but not after bit 0).
        bool bit = x >> k & 1;
        putchar(bit ? '1' : '0');
        if ((k % 4 == 0) && (k!=0) ){
            putchar(' ');
        }
    }
}

// x with bit k set to 1
static uint32_t bit_set(uint32_t x, uint32_t k) {
    // TODO 2
    x = x | (1u<<k);
    return x;
}

// x with bit k cleared to 0
static uint32_t bit_clear(uint32_t x, uint32_t k) {
    // TODO 3
    x = x & (~(1u<<k));
    return x;
}

// x with bit k flipped
static uint32_t bit_toggle(uint32_t x, uint32_t k) {
    // TODO 4
    x = x ^ (1u<<k);
    return x;
}

// true if bit k of x is 1
static bool bit_test(uint32_t x, uint32_t k) {
    // TODO 5
    bool result = false;
    if((x>>k) & 1u){
        result = true;
    }
    return result;
}

// A mask with `width` ones starting at bit `lsb`: field_mask(4, 12) = 0x0000FFF0. Width 1 … 31.
static uint32_t field_mask(uint32_t lsb, uint32_t width) {
    // TODO 6
    return ((1u<<width) - 1u) << lsb;
}

// The number stored in bits lsb … lsb + width − 1 of x, moved down to bit 0
static uint32_t field_get(uint32_t x, uint32_t lsb, uint32_t width) {
    // TODO 7: use field_mask()
    uint32_t mask = field_mask(lsb, width);
    return (x&mask)>>lsb;
}

// x with that field replaced by `value`. The other bits of x stay as they were.
static uint32_t field_set(uint32_t x, uint32_t lsb, uint32_t width, uint32_t value) {
    // TODO 8: clear the field in x, then OR in the value, shifted up and cut to fit the field
    uint32_t mask = field_mask(lsb, width);
    x = (x & ~mask) | ((value << lsb) & mask);
    return x;
}

// --- Self-test: like a tiny pytest. Every check FAILs until you finish its TODO. ---

static int failures;

static void check(const char *what, uint32_t got, uint32_t want) {
    if (got == want) {
        printf("  PASS  %s\n", what);
    } else {
        printf("  FAIL  %s: got 0x%08lx, want 0x%08lx\n", what, got, want);
        failures++;
    }
}

// For datasheet numbers: on a FAIL it doesn't show the right answer, so you still have to look it up.
static void check_ds(const char *what, uint32_t got, uint32_t want) {
    if (got == want) {
        printf("  PASS  %s\n", what);
    } else {
        printf("  FAIL  %s: you have 0x%08lx\n", what, got);
        failures++;
    }
}

static void run_checks(void) {
    failures = 0;
    printf("Part A checks\n");
    check("bit_set(0x00, 3)",            bit_set(0x00, 3),            0x08);
    check("bit_set(0x08, 3) (already 1)", bit_set(0x08, 3),           0x08);
    check("bit_set(0, 31)",              bit_set(0, 31),              0x80000000);
    check("bit_clear(0xFF, 7)",          bit_clear(0xFF, 7),          0x7F);
    check("bit_clear(0x7F, 7) (already 0)", bit_clear(0x7F, 7),       0x7F);
    check("bit_toggle(0x56, 6)",         bit_toggle(0x56, 6),         0x16);
    check("bit_toggle(0x16, 6)",         bit_toggle(0x16, 6),         0x56);
    check("bit_test(0x56, 6)",           bit_test(0x56, 6),           1);
    check("bit_test(0x56, 0)",           bit_test(0x56, 0),           0);
    check("field_mask(0, 4)",            field_mask(0, 4),            0x0000000F);
    check("field_mask(4, 12)",           field_mask(4, 12),           0x0000FFF0);
    check("field_get(0x410CC601, 4, 12)", field_get(0x410CC601, 4, 12), 0xC60);
    check("field_get(0x56, 4, 2)",       field_get(0x56, 4, 2),       1);
    check("field_set(0x56, 4, 2, 3)",    field_set(0x56, 4, 2, 3),    0x76);
    check("field_set(0xFFFFFFFF, 8, 8, 0)", field_set(0xFFFFFFFF, 8, 8, 0), 0xFFFF00FF);
    check("field_set(0, 0, 4, 0x1F) (too big)", field_set(0, 0, 4, 0x1F), 0x0F);
    printf("%d failed\n\n", failures);
}

// Predict each line on paper before you run it. Python would print something different for two of them.
static void warm_up(void) {
    uint32_t x = 0x56;
    printf("Warm-up\n");
    printf("x       = "); print_bin(x);        printf("\n");
    printf("~x      = "); print_bin(~x);       printf("\n");
    printf("x << 4  = "); print_bin(x << 4);   printf("\n");
    printf("x >> 4  = "); print_bin(x >> 4);   printf("\n");
    printf("x & 0xF = "); print_bin(x & 0xF);  printf("\n");
    printf("x | 1   = "); print_bin(x | 1);    printf("\n");
    printf("x ^ x   = "); print_bin(x ^ x);    printf("\n");
    printf("0u - 1  = "); print_bin(0u - 1);   printf("\n\n");
}

// --- Part B: real registers ---

static void decode_registers(void) {
    failures = 0;
    printf("Part B datasheet checks\n");
    check_ds("D1 CPUID address",   CPUID_ADDR,   PPB_BASE + M0PLUS_CPUID_OFFSET);
    check_ds("D1 CHIP_ID address", CHIP_ID_ADDR, SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    if (failures != 0) {
        printf("Not reading any register until D1 is right: a wrong address can crash the chip.\n");
        return;
    }

    // TODO 9: read the CPUID register through a pointer:
    //         volatile uint32_t *p = (volatile uint32_t *)CPUID_ADDR;  then  cpuid = *p;
    volatile uint32_t *p = (volatile uint32_t *)CPUID_ADDR;
    uint32_t cpuid = *p;
    printf("CPUID   @ 0x%08lx = 0x%08lx = ", (uint32_t)CPUID_ADDR, cpuid);
    print_bin(cpuid);
    printf("\n");

    // TODO 10: find the CPUID register's table in the datasheet. Decode all 5 fields with field_get(),
    //          using the bit ranges from the table, and compare each with the table's Reset column.
    //          Print the implementer also as a character with %c. What letter is it?
    printf("IMPLEMENTER  0x%lx\n", field_get(cpuid, 24, 8));
    printf("VARIANT:     0x%lx\n",   field_get(cpuid, 20, 4));
    printf("ARCH:        0x%lx\n",   field_get(cpuid, 16, 4));
    printf("PARTNO:      0x%lx\n", field_get(cpuid, 4, 12));
    printf("REVISION:    0x%lx\n",   field_get(cpuid, 0, 4));

    // TODO 11: read CHIP_ID the same way. Find its table, then decode it with the SDK's own names
    //          instead of field_get(): (chip_id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB,
    //          and the same for MANUFACTURER and REVISION. Print each field in hex.
    volatile uint32_t *t = (volatile uint32_t *)CHIP_ID_ADDR;
    uint32_t chipid = *t;
    printf("CHIP ID   @ 0x%08lx = 0x%08lx = ", (uint32_t)CHIP_ID_ADDR, chipid);
    print_bin(chipid);
    printf("\n");
    printf("REVISION  0x%lx\n", field_get(chipid, 28, 6));
    printf("PART:     0x%lx\n",   field_get(chipid, 12, 16));
    printf("MANUFACTURER:        0x%lx\n",   field_get(chipid, 12, 0));

}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);     // give the serial monitor time to connect

    while (true) {
        printf("=== Lab C1 ===\n");
        warm_up();
        run_checks();
        decode_registers();
        printf("\n");
        sleep_ms(5000);
    }
}
