add_executable(lab07_i2s_dac
    main.c
)

# Runs pioasm on i2s.pio at build time and creates i2s.pio.h, which main.c includes.
pico_generate_pio_header(lab07_i2s_dac ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)

target_link_libraries(lab07_i2s_dac PRIVATE
    pico_stdlib     # GPIO, printf, sinf()/powf() via pico_float, clock_get_hz()
    hardware_pio    # the I2S state machine
    hardware_dma    # Part C: DMA feeds the PIO
    hardware_irq    # Part C: our DMA interrupt handler
)

pico_enable_stdio_usb(lab07_i2s_dac 1)
pico_enable_stdio_uart(lab07_i2s_dac 0)

pico_add_extra_outputs(lab07_i2s_dac)
