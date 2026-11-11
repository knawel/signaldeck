add_executable(lab06_sigma_delta
    main.c
)

# Runs pioasm on sigma_delta.pio at build time and creates sigma_delta.pio.h,
# which main.c includes.
pico_generate_pio_header(lab06_sigma_delta ${CMAKE_CURRENT_LIST_DIR}/sigma_delta.pio)

target_link_libraries(lab06_sigma_delta PRIVATE
    pico_stdlib     # GPIO, printf, sinf() via pico_float, clock_get_hz()
    hardware_pio    # PIO blocks and state machines
    hardware_adc    # measuring FILT on GP26
)

pico_enable_stdio_usb(lab06_sigma_delta 1)
pico_enable_stdio_uart(lab06_sigma_delta 0)

pico_add_extra_outputs(lab06_sigma_delta)
