add_executable(lab04_dma_feed
    main.c
)

target_link_libraries(lab04_dma_feed PRIVATE
    pico_stdlib     # GPIO, printf, repeating timers, sinf() via pico_float, clock_get_hz()
    hardware_pwm    # the PWM slices
    hardware_dma    # DMA channels and their pacing timers
    hardware_irq    # installing our own interrupt handler
)

pico_enable_stdio_usb(lab04_dma_feed 1)
pico_enable_stdio_uart(lab04_dma_feed 0)

pico_add_extra_outputs(lab04_dma_feed)
