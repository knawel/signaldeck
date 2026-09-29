add_executable(lab00_hello
    main.c
)

target_link_libraries(lab00_hello PRIVATE
    pico_stdlib
    hardware_clocks
)

# Labs don't use USB audio, so printf goes over USB (shows up as a serial port)
pico_enable_stdio_usb(lab00_hello 1)
pico_enable_stdio_uart(lab00_hello 0)

# Generate .uf2 (drag-and-drop flashing)
pico_add_extra_outputs(lab00_hello)
