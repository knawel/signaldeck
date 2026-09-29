add_executable(lab01_square_tone
    main.c
)

target_link_libraries(lab01_square_tone PRIVATE
    pico_stdlib     # includes pico_time: sleep_us, repeating timers
)

pico_enable_stdio_usb(lab01_square_tone 1)
pico_enable_stdio_uart(lab01_square_tone 0)

pico_add_extra_outputs(lab01_square_tone)
