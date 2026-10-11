add_executable(labc1_bits
    main.c
)

target_link_libraries(labc1_bits PRIVATE
    pico_stdlib     # printf over USB, sleep_ms, and the SDK's register address headers
)

pico_enable_stdio_usb(labc1_bits 1)
pico_enable_stdio_uart(labc1_bits 0)

pico_add_extra_outputs(labc1_bits)
