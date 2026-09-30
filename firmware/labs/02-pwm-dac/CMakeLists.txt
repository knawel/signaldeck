add_executable(lab02_pwm_dac
    main.c
)

target_link_libraries(lab02_pwm_dac PRIVATE
    pico_stdlib     # GPIO, printf, repeating timers
    hardware_pwm    # the PWM slices
    hardware_adc    # reading our own output back on GP26
    hardware_clocks # clock_get_hz()
)

pico_enable_stdio_usb(lab02_pwm_dac 1)
pico_enable_stdio_uart(lab02_pwm_dac 0)

pico_add_extra_outputs(lab02_pwm_dac)
