cmake_minimum_required(VERSION 3.16)

# Skip ABI detection tests to avoid log clutter
set(CMAKE_C_ABI_COMPILED 1)
set(CMAKE_CXX_ABI_COMPILED 1)

# Project definition
project(MyOS ASM C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Kernel code must never be position-independent; this prevents CMake from
# injecting -fPIC into OBJECT library targets, which conflicts with -mcmodel=kernel
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Override ASM compile rule to suppress unwanted default flags
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")

# Kernel compile flags
add_compile_options(
    $<$<COMPILE_LANGUAGE:C,CXX>:-m64>
    $<$<COMPILE_LANGUAGE:C,CXX>:-march=x86-64>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mcmodel=kernel>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mno-red-zone>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mno-80387>
    $<$<COMPILE_LANGUAGE:C,CXX>:-mno-mmx>
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-stack-protector>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-pie>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-PIC>
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:C,CXX>:-g>
    $<$<COMPILE_LANGUAGE:C,CXX>:-O1>
    $<$<COMPILE_LANGUAGE:ASM>:--64>
)

# C++ specific flags
add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
)

# Sysroot configuration
set(SYSROOT "${CMAKE_SOURCE_DIR}/sysroot" CACHE PATH "Sysroot directory")

# Include directories (before add_subdirectory so subdirs inherit them)
include_directories(${SYSROOT}/usr/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add kernel subdirectory — also sets LINKER_SCRIPT in this scope via PARENT_SCOPE
add_subdirectory(src/kernel)

# Add userspace programs (compiled separately, output to initramfs/bin)
add_subdirectory(src/user)

# Create the kernel using a custom command with explicit link order
# Note: boot_objs, crti_obj, crtn_obj, and kernel_objs are all defined in src/kernel/
file(MAKE_DIRECTORY ${SYSROOT}/boot)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/kernel.elf ${CMAKE_BINARY_DIR}/kernel.map ${CMAKE_BINARY_DIR}/.kernel_stamp
    COMMAND ${CMAKE_CXX_COMPILER}
        -m64
        -T ${LINKER_SCRIPT}
        -nostdlib
        -static
        -no-pie
        -Wl,-Map=kernel.map
        -Wl,--build-id=none
        -z noexecstack
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
