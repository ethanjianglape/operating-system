cmake_minimum_required(VERSION 3.16)

# Our cross compiler i686-elf-gcc is not able to compile a standard C file
# at the moment, so this step needs to be skipped in cmake
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Project definition
project(MyOS ASM C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable ASM with NASM
# enable_language(ASM_GAS)

# Set NASM flags for 64-bit ELF format
set(CMAKE_ASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")

# Compiler flags specific to kernel development (C and C++ only, not ASM)
add_compile_options(
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-stack-protector>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-pic>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mno-80387>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mno-mmx>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mno-sse>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mno-sse2>
)

# C++ specific flags
add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
)

# Get crtbegin.o and crtend.o from our compiler
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -print-file-name=crtbegin.o
    OUTPUT_VARIABLE CRTBEGIN_OBJ
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -print-file-name=crtend.o
    OUTPUT_VARIABLE CRTEND_OBJ
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(BOOT_SOURCES
     src/boot/boot.s
)

set(KERNEL_SOURCES
    src/kernel/kernel.cpp
    src/kernel/vga.cpp
)

set(CRT_SOURCES
    src/crt/crti.s
    src/crt/crtn.s
)

# Create object libraries for proper link order control
add_library(crti_obj OBJECT src/crt/crti.s)
add_library(crtn_obj OBJECT src/crt/crtn.s)
add_library(boot_objs OBJECT ${BOOT_SOURCES})
add_library(kernel_objs OBJECT ${KERNEL_SOURCES})

# Apply the same compile options to object libraries
target_compile_options(boot_objs PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-stack-protector>
)

target_compile_options(kernel_objs PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-stack-protector>
)

# Create the kernel using a custom command with explicit link order
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/kernel.elf ${CMAKE_BINARY_DIR}/kernel.map
    COMMAND ${CMAKE_CXX_COMPILER}
        -T ${CMAKE_SOURCE_DIR}/linker.ld
        -nostdlib
        -static
        -Wl,-Map=kernel.map
        -o kernel.elf
        "$<TARGET_OBJECTS:crti_obj>"
        ${CRTBEGIN_OBJ}
        "$<TARGET_OBJECTS:boot_objs>"
        "$<TARGET_OBJECTS:kernel_objs>"
        ${CRTEND_OBJ}
        "$<TARGET_OBJECTS:crtn_obj>"
        -lgcc
    DEPENDS boot_objs kernel_objs
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Linking kernel.elf with CRT objects: crti -> crtbegin -> [boot] -> [kernel] -> crtend -> crtn"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

add_custom_target(kernel ALL DEPENDS ${CMAKE_BINARY_DIR}/kernel.elf)

# Custom command to create kernel binary
add_custom_command(TARGET kernel POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary kernel.elf kernel.bin
    COMMENT "Creating kernel binary"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Print build information
message(STATUS "Building kernel for i686-elf (32-bit) architecture")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "ASM Assembler: ${CMAKE_ASM_COMPILER}")
message(STATUS "crtbegin.o Location: ${CRTBEGIN_OBJ}")
message(STATUS "crtend.o Location: ${CRTEND_OBJ}")
