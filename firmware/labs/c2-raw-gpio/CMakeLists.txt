add_executable(labc2_raw_gpio
    main.c
)

target_link_libraries(labc2_raw_gpio PRIVATE
    pico_stdlib     # printf over USB, sleep_ms, and the SDK's register address headers
)

pico_enable_stdio_usb(labc2_raw_gpio 1)
pico_enable_stdio_uart(labc2_raw_gpio 0)

pico_add_extra_outputs(labc2_raw_gpio)
