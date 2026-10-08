add_executable(lab05_r2r_ladder
    main.c
)

target_link_libraries(lab05_r2r_ladder PRIVATE
    pico_stdlib     # GPIO, printf, repeating timers, sinf() via pico_float
    hardware_adc    # measuring the ladder output on GP27
)

pico_enable_stdio_usb(lab05_r2r_ladder 1)
pico_enable_stdio_uart(lab05_r2r_ladder 0)

pico_add_extra_outputs(lab05_r2r_ladder)
