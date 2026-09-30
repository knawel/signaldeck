add_executable(lab03_sine_dds
    main.c
)

target_link_libraries(lab03_sine_dds PRIVATE
    pico_stdlib     # GPIO, printf, repeating timers, sinf() via pico_float
    hardware_pwm    # the PWM slices
)

pico_enable_stdio_usb(lab03_sine_dds 1)
pico_enable_stdio_uart(lab03_sine_dds 0)

pico_add_extra_outputs(lab03_sine_dds)
