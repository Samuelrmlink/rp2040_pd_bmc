cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(rp2040_bmc_pd C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(bmc_pd)

pico_generate_pio_header(bmc_pd ${CMAKE_CURRENT_LIST_DIR}/src/bmc.pio)

target_sources(bmc_pd PRIVATE src/main.c)

target_link_libraries(bmc_pd PRIVATE pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(bmc_pd)
pico_enable_stdio_usb(bmc_pd 1)
pico_enable_stdio_uart(bmc_pd 0)
