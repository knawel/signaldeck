add_executable(lab08_usb_audio
    main.c
    usb_descriptors.c
)

# Runs pioasm on your lab 7 I2S program and creates i2s.pio.h, which main.c includes.
pico_generate_pio_header(lab08_usb_audio ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)

# TinyUSB looks for "tusb_config.h": this makes it find ours in this folder.
target_include_directories(lab08_usb_audio PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# We drive TinyUSB ourselves, so the SDK no longer adds the interface picotool uses to reboot
# the board. These put it back (usb_descriptors.c lists it), so "Run" keeps working.
target_compile_definitions(lab08_usb_audio PRIVATE
    PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE=1
    PICO_ENABLE_USB_RESET_VIA_BAUD_RATE=1
    PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR=0
)

target_link_libraries(lab08_usb_audio PRIVATE
    pico_stdlib     # GPIO, printf, clock_get_hz()
    hardware_pio    # the I2S state machine
    hardware_dma    # DMA feeds the PIO
    hardware_irq    # our DMA interrupt handler
    tinyusb_device  # the USB device stack: we write the descriptors and callbacks
    pico_unique_id  # the serial number string
    pico_usb_reset  # the picotool reset interface
)

# printf still goes over USB: the serial port is one of our own USB interfaces now.
pico_enable_stdio_usb(lab08_usb_audio 1)
pico_enable_stdio_uart(lab08_usb_audio 0)

pico_add_extra_outputs(lab08_usb_audio)
