cmake_minimum_required(VERSION 3.13)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Write build/compile_commands.json so VS Code IntelliSense sees the real compiler flags and include paths
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Tells the SDK our board's flash size and pin defaults. Must be set before the SDK is imported.
set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")

# Must come before project()
include(pico_sdk_import.cmake)

project(signaldeck C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_compile_options(-Wall -Wextra)

add_executable(signaldeck
    src/main.c
    src/usb_descriptors.c
)

# Runs pioasm on the I2S program and creates i2s.pio.h, which main.c includes.
pico_generate_pio_header(signaldeck ${CMAKE_CURRENT_LIST_DIR}/src/i2s.pio)

# TinyUSB looks for "tusb_config.h": this makes it find ours in src/.
target_include_directories(signaldeck PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

# We drive TinyUSB ourselves, so the SDK no longer adds the interface picotool uses to reboot
# the board. These put it back (usb_descriptors.c lists it), so flashing without BOOT works.
target_compile_definitions(signaldeck PRIVATE
    PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE=1
    PICO_ENABLE_USB_RESET_VIA_BAUD_RATE=1
    PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR=0
)

target_link_libraries(signaldeck PRIVATE
    pico_stdlib     # GPIO, printf, clock_get_hz()
    hardware_pio    # the I2S state machine
    hardware_dma    # DMA feeds the PIO
    hardware_irq    # our DMA interrupt handler
    tinyusb_device  # the USB device stack: we write the descriptors and callbacks
    pico_unique_id  # the serial number string
    pico_usb_reset  # the picotool reset interface
)

# printf goes over USB: the serial port is one of our own USB interfaces.
pico_enable_stdio_usb(signaldeck 1)
pico_enable_stdio_uart(signaldeck 0)

pico_add_extra_outputs(signaldeck)
