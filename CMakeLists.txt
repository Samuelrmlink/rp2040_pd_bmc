cmake_minimum_required(VERSION 3.13)

# Include the Pico SDK
include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(FREERTOS_CFG_DIRECTORY "${CMAKE_SOURCE_DIR}/FreeRTOS-Config")
set(FREERTOS_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/FreeRTOS-Kernel")

# Define this project
project(rp2040_bmc_pd C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# Add FreeRTOS as a library
add_library(FreeRTOS STATIC
    ${FREERTOS_SRC_DIRECTORY}/event_groups.c
    ${FREERTOS_SRC_DIRECTORY}/list.c
    ${FREERTOS_SRC_DIRECTORY}/queue.c
    ${FREERTOS_SRC_DIRECTORY}/stream_buffer.c
    ${FREERTOS_SRC_DIRECTORY}/tasks.c
    ${FREERTOS_SRC_DIRECTORY}/timers.c
    ${FREERTOS_SRC_DIRECTORY}/portable/MemMang/heap_3.c
    ${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0/port.c
)

# Build FreeRTOS
target_include_directories(FreeRTOS PUBLIC
    ${FREERTOS_CFG_DIRECTORY}/
    ${FREERTOS_SRC_DIRECTORY}/include
    ${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0
)

# Add bmc_pd source code
file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(bmc_pd ${SOURCES})

# Add bmc RP2 PIO interface
pico_generate_pio_header(bmc_pd ${CMAKE_CURRENT_LIST_DIR}/src/bmc.pio)

# Link libraries used for this application
target_link_libraries(bmc_pd PRIVATE pico_stdlib hardware_pio hardware_dma FreeRTOS)

# Add debug & other compiler options
add_compile_options(-Wall
	-Wno-format
	#-wno-unused-function
	)

# Generate additional output files (.UF2)
pico_add_extra_outputs(bmc_pd)

# Enable console output
pico_enable_stdio_usb(bmc_pd 1)
pico_enable_stdio_uart(bmc_pd 1)
