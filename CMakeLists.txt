cmake_minimum_required(VERSION 3.16)

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

# Set assembler flags for 64-bit ELF format
set(CMAKE_ASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")

# Compiler flags specific to kernel development (C and C++ only, not ASM)
add_compile_options(
    $<$<COMPILE_LANGUAGE:C,CXX>:-m64>
    $<$<COMPILE_LANGUAGE:C,CXX>:-march=x86-64>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mcmodel=kernel>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mno-red-zone>
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

# Sysroot configuration
set(SYSROOT "${CMAKE_SOURCE_DIR}/sysroot" CACHE PATH "Sysroot directory")

# Add libc subdirectory FIRST (this installs headers to sysroot)
#add_subdirectory(src/libc)

# Add kernel subdirectory (this installs kernel headers to sysroot)
add_subdirectory(src/kernel)

# Add userspace programs (compiled separately, output to initramfs/bin)
add_subdirectory(src/user)

# Include directories (sysroot comes first for libc headers)
include_directories(${SYSROOT}/usr/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create the kernel using a custom command with explicit link order
# Note: boot_objs, crti_obj, crtn_obj, and kernel_objs are all defined in src/kernel/
file(MAKE_DIRECTORY ${SYSROOT}/boot)

# Force kernel to always be considered out of date to ensure sysroot updates
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/kernel.elf ${CMAKE_BINARY_DIR}/kernel.map ${CMAKE_BINARY_DIR}/.kernel_stamp
    COMMAND ${CMAKE_CXX_COMPILER}
        -m64
        -T ${CMAKE_SOURCE_DIR}/src/kernel/arch/x86_64/limine.ld
        -nostdlib
        -static
        -Wl,-Map=kernel.map
        -Wl,--build-id=none
        -o kernel.elf
        "$<TARGET_OBJECTS:crti_obj>"
        "$<TARGET_OBJECTS:boot_objs>"
        "$<TARGET_OBJECTS:kernel_objs>"
        "$<TARGET_OBJECTS:crtn_obj>"
        -lgcc
    COMMAND ${CMAKE_OBJCOPY} -O binary kernel.elf kernel.bin
    COMMAND ${CMAKE_COMMAND} -E remove -f ${SYSROOT}/boot/kernel.elf
    COMMAND ${CMAKE_COMMAND} -E remove -f ${SYSROOT}/boot/kernel.bin
    COMMAND ${CMAKE_COMMAND} -E copy kernel.elf ${SYSROOT}/boot/kernel.elf
    COMMAND ${CMAKE_COMMAND} -E copy kernel.bin ${SYSROOT}/boot/kernel.bin
    COMMAND ${CMAKE_COMMAND} -E touch .kernel_stamp
    DEPENDS boot_objs kernel_objs crti_obj crtn_obj
        $<TARGET_OBJECTS:boot_objs>
        $<TARGET_OBJECTS:kernel_objs>
        $<TARGET_OBJECTS:crti_obj>
        $<TARGET_OBJECTS:crtn_obj>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Linking kernel, creating binary, and copying to sysroot"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

add_custom_target(kernel ALL DEPENDS ${CMAKE_BINARY_DIR}/kernel.elf)

# Print build information
message(STATUS "Building kernel for x86_64 (64-bit) architecture using standard gcc")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "ASM Assembler: ${CMAKE_ASM_COMPILER}")
