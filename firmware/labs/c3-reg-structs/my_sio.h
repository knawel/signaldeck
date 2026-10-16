// Lab C3 — your own register map for the start of the SIO block, the way the SDK writes them.
// Compare with the SDK's version: hardware_structs/include/hardware/structs/sio.h

#ifndef MY_SIO_H
#define MY_SIO_H

#include <stdint.h>
#include <stddef.h>     // offsetof

// One field per register, in address order. Each field is 4 bytes, so its offset in the
// struct is its offset from the start of the block. Source: the datasheet's list of SIO registers.
typedef struct {
    volatile uint32_t cpuid;        // 0x000  which core is reading: 0 or 1
    volatile uint32_t gpio_in;      // 0x004
    volatile uint32_t gpio_hi_in;   // 0x008  the QSPI flash pins
    volatile uint32_t _pad0; 
    volatile uint32_t gpio_out;
    volatile uint32_t gpio_set;
    volatile uint32_t gpio_clr;
    volatile uint32_t gpio_togl;
    volatile uint32_t gpio_oe;
    volatile uint32_t gpio_oe_set;
    volatile uint32_t gpio_oe_clr;

    // TODO 1: continue from the datasheet's list of SIO registers, up to and including GPIO_OE_CLR.
    //         Use the SDK's names: gpio_out, gpio_set, gpio_clr, gpio_togl, gpio_oe, gpio_oe_set, gpio_oe_clr.
    //         Read the Offset column, not just the names.
} my_sio_t;

// "At address 0xD0000000 there is a my_sio_t." Use it as MY_SIO->gpio_out.
#define MY_SIO  ((my_sio_t *)0xD0000000u)

// The compiler checks the layout against the datasheet. A wrong field order won't build.
_Static_assert(offsetof(my_sio_t, gpio_hi_in) == 0x008, "gpio_hi_in must be at 0x008");
_Static_assert(offsetof(my_sio_t, gpio_out) == 0x010, "gpio_out must be at 0x010");
_Static_assert(offsetof(my_sio_t, gpio_togl) == 0x01c, "gpio_togl must be at 0x01c");
_Static_assert(offsetof(my_sio_t, gpio_oe_clr) == 0x028, "gpio_oe_clr must be at 0x038");

// TODO 2: the same check for gpio_out, gpio_togl and gpio_oe_clr, with the offsets from the datasheet

#endif
