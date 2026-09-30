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

project(signaldeck_labs C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Let the compiler point out suspicious code — the SDK itself builds clean with these
add_compile_options(-Wall -Wextra)

# One executable per lab — add a line here when starting a new lab
add_subdirectory(00-hello)
add_subdirectory(01-square-tone)
add_subdirectory(02-pwm-dac)
