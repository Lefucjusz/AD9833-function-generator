cmake_minimum_required(VERSION 3.22)

# Project name
project(AD9833)

# Language configuration
enable_language(C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(PROJ_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# Executable name
set(EXECUTABLE ${CMAKE_PROJECT_NAME})

# Linker script
set(LDSCRIPT_PATH ${PROJ_PATH}/CH32V00x.ld)

# Source files
set(SRC_FILES
    ${PROJ_PATH}/drivers/core/core_riscv.c
    ${PROJ_PATH}/drivers/delay/delay.c
    ${PROJ_PATH}/drivers/gpio/gpio.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_adc.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_dbgmcu.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_dma.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_exti.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_flash.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_gpio.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_i2c.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_iwdg.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_misc.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_opa.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_pwr.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_rcc.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_spi.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_tim.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_usart.c
    ${PROJ_PATH}/drivers/hal/ch32v00x_wwdg.c
    ${PROJ_PATH}/system/system_ch32v00x.c
    ${PROJ_PATH}/startup/startup_ch32v00x.S
    ${PROJ_PATH}/main.c

    ${PROJ_PATH}/encoder/encoder.c
    ${PROJ_PATH}/hd44780/hd44780_io.c
    ${PROJ_PATH}/hd44780/hd44780.c
)

# Include directories
set(INCLUDE_DIRS
    ${PROJ_PATH}/drivers/core
    ${PROJ_PATH}/drivers/delay
    ${PROJ_PATH}/drivers/gpio
    ${PROJ_PATH}/drivers/hal
    ${PROJ_PATH}/system

    ${PROJ_PATH}/encoder
    ${PROJ_PATH}/hd44780
)

# Executable
add_executable(${EXECUTABLE} ${SRC_FILES})

# Include paths
target_include_directories(${EXECUTABLE} 
    PRIVATE
        ${INCLUDE_DIRS}
)

# CPU options
set(CPU_OPTIONS
    -march=rv32ec_zicsr
    -mabi=ilp32e
)

# Compile options
target_compile_options(${EXECUTABLE}
    PRIVATE
        ${CPU_OPTIONS}
        -msmall-data-limit=0
        -msave-restore
        -fmessage-length=0
        -fsigned-char
        -ffunction-sections
        -fdata-sections
        -fno-common
)

# Set standard CMake build type options
set(CMAKE_C_FLAGS_DEBUG_INIT "-g3 -O0 -Wall -Wextra -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG_INIT}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE_INIT "-O3 -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE_INIT}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os -Wall")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL_INIT}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O1 -g -Wall")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_ASM_FLAGS_RELWITHDEBINFO_INIT}" CACHE STRING "" FORCE)

message("Build type: " ${CMAKE_BUILD_TYPE})

# Linker options
target_link_options(${EXECUTABLE}
    PRIVATE
        -T${LDSCRIPT_PATH}
        ${CPU_OPTIONS}
        -nostartfiles
        --specs=nano.specs
        -Wl,--gc-sections
	    -Wl,--print-memory-usage
        -Wl,-Map=${CMAKE_PROJECT_NAME}.map
)
