cmake_minimum_required(VERSION 3.16)

# Our cross compiler i686-elf-gcc is not able to compile a standard C file
# at the moment, so compiler checks need to be skipped
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Skip ABI detection tests to avoid log clutter
set(CMAKE_C_ABI_COMPILED 1)
set(CMAKE_CXX_ABI_COMPILED 1)

# Project definition
project(MyOS ASM C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# Sysroot configuration
set(SYSROOT "${CMAKE_SOURCE_DIR}/sysroot" CACHE PATH "Sysroot directory")

# Add libc subdirectory FIRST (this installs headers to sysroot)
add_subdirectory(src/libc)

# Add kernel subdirectory (this installs kernel headers to sysroot)
add_subdirectory(src/kernel)

# Include directories (sysroot comes first for libc headers)
include_directories(${SYSROOT}/usr/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create the kernel using a custom command with explicit link order
# Note: boot_objs, crti_obj, crtn_obj, and kernel_objs are all defined in src/kernel/
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/kernel.elf ${CMAKE_BINARY_DIR}/kernel.map
    COMMAND ${CMAKE_CXX_COMPILER}
        -T ${CMAKE_SOURCE_DIR}/src/kernel/arch/i386/linker.ld
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
        "$<TARGET_FILE:libk>"
        -lgcc
    DEPENDS boot_objs kernel_objs crti_obj crtn_obj libk
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

# Copy kernel to sysroot/boot
file(MAKE_DIRECTORY ${SYSROOT}/boot)
add_custom_command(TARGET kernel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy kernel.elf ${SYSROOT}/boot/kernel.elf
    COMMAND ${CMAKE_COMMAND} -E copy kernel.bin ${SYSROOT}/boot/kernel.bin
    COMMENT "Copying kernel to sysroot/boot"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Print build information
message(STATUS "Building kernel for i686-elf (32-bit) architecture")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "ASM Assembler: ${CMAKE_ASM_COMPILER}")
message(STATUS "crtbegin.o Location: ${CRTBEGIN_OBJ}")
message(STATUS "crtend.o Location: ${CRTEND_OBJ}")
