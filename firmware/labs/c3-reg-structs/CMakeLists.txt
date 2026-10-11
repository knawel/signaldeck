add_executable(labc3_reg_structs
    main.c
)

target_link_libraries(labc3_reg_structs PRIVATE
    pico_stdlib     # printf over USB, sleep_ms, and the SDK's register address headers
)

pico_enable_stdio_usb(labc3_reg_structs 1)
pico_enable_stdio_uart(labc3_reg_structs 0)

pico_add_extra_outputs(labc3_reg_structs)
