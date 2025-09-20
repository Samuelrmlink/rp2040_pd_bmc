# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Library Base Paths
set(CONFIG_BASE_PATH ${CMAKE_SOURCE_DIR}/config)
set(CMAKE_INCLUDES_PATH ${CONFIG_BASE_PATH}/cmake)
set(LIBRARY_BASE_PATH ${CMAKE_SOURCE_DIR}/lib)
set(PROJECT_INCLUDES_PATH ${CMAKE_SOURCE_DIR}/include)

if(NOT DEFINED FREERTOS_KERNEL_PATH)
    set(FREERTOS_KERNEL_PATH ${LIBRARY_BASE_PATH}/FreeRTOS-Kernel)
    message(STATUS "FREERTOS_KERNEL_PATH = ${FREERTOS_KERNEL_PATH}")
endif()

# Include the Pico SDK
include(${CMAKE_INCLUDES_PATH}/pico_sdk_import.cmake)

# Include FreeRTOS
include(${CMAKE_INCLUDES_PATH}/FreeRTOS_Kernel_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Define this project
project(bmc_pd C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# Add bmc_pd source code
file(GLOB_RECURSE SOURCES "src/*.c*" "src/cli/*.c*")
add_executable(bmc_pd ${SOURCES})

# Add bmc RP2 PIO interface
pico_generate_pio_header(bmc_pd ${CMAKE_CURRENT_LIST_DIR}/src/bmc.pio)

# Link libraries used for this application
target_link_libraries(bmc_pd PRIVATE pico_stdlib pico_unique_id hardware_pio hardware_dma pico_async_context_freertos FreeRTOS-Kernel-Heap4)
target_include_directories(bmc_pd PRIVATE ${CONFIG_BASE_PATH}/FreeRTOS ${PROJECT_INCLUDES_PATH})

# Add debug & other compiler options
add_compile_options(-Wall
	-Wno-format
	#-wno-unused-function
	)

# Generate additional output files (.UF2)
pico_add_extra_outputs(bmc_pd)

# Enable console output
if(NOT(PICO_PLATFORM STREQUAL "rp2350-riscv"))
    pico_enable_stdio_usb(bmc_pd 1)
endif()
pico_enable_stdio_uart(bmc_pd 1)

# Passthrough PICO_PLATFORM to compiler
set(FW_PLATFORM "${PICO_PLATFORM}")

# Build Date
string(TIMESTAMP FW_BUILD_DATE "%Y-%m-%d")
# Commit
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE FW_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE FW_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Tag
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE FW_GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if("${FW_GIT_TAG}" STREQUAL "")
    set(FW_GIT_TAG "none")
endif()

set(INFO_FLAGS
    -DFW_PLATFORM="${FW_PLATFORM}"
    -DFW_BUILD_DATE="${FW_BUILD_DATE}"
    -DFW_GIT_COMMIT="${FW_GIT_COMMIT}"
    -DFW_GIT_BRANCH="${FW_GIT_BRANCH}"
    -DFW_GIT_TAG="${FW_GIT_TAG}"
)
set_property(SOURCE "src/cli/cli_command_device_info.cpp" APPEND PROPERTY COMPILE_OPTIONS ${INFO_FLAGS})
