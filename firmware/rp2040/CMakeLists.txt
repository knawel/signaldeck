cmake_minimum_required(VERSION 3.13)

# Must come before project()
include(pico_sdk_import.cmake)

project(signaldeck C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Compile i2s.pio → i2s.pio.h (included by i2s_audio.c)
pico_generate_pio_header(signaldeck ${CMAKE_CURRENT_LIST_DIR}/src/i2s.pio)

add_executable(signaldeck
    src/main.c
    src/usb_descriptors.c
    src/usb_audio.c
    src/i2s_audio.c
)

target_include_directories(signaldeck PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(signaldeck PRIVATE
    pico_stdlib
    pico_unique_id
    hardware_pio
    hardware_dma
    hardware_irq
    tinyusb_device
    tinyusb_board
)

# USB is used for audio — keep UART for debug printf
pico_enable_stdio_uart(signaldeck 1)
pico_enable_stdio_usb(signaldeck 0)

# Generate .uf2 and other flash formats
pico_add_extra_outputs(signaldeck)
