cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(kernel C CXX ASM)

# Architecture-specific settings
set(KERNEL_ARCH "x86_64" CACHE STRING "Target architecture")
set(ARCH_DIR "arch/${KERNEL_ARCH}")
set(ARCH_DRIVERS_DIR "arch/${KERNEL_ARCH}/drivers")
set(LIB_DIR "lib/")
set(TEST_DIR "test/")

# Kernel source files
set(KERNEL_SOURCES
    kernel.cpp
)

# Architecture-specific kernel sources
set(ARCH_KERNEL_SOURCES
    ${ARCH_DIR}/gdt/gdt.cpp
    ${ARCH_DIR}/gdt/gdt.s
    ${ARCH_DIR}/interrupts/idt.cpp
    ${ARCH_DIR}/interrupts/irq.cpp
    ${ARCH_DIR}/interrupts/isr.s
    ${ARCH_DIR}/syscall/syscall.cpp
    ${ARCH_DIR}/syscall/syscall_entry.s
    ${ARCH_DIR}/syscall/int80_entry.s
    ${ARCH_DIR}/memory/vmm.cpp
    ${ARCH_DIR}/process/process.cpp
    #${ARCH_DIR}/process/process.s
)

set(ARCH_DRIVER_KERNEL_SOURCES
    ${ARCH_DRIVERS_DIR}/pic/pic.cpp
    ${ARCH_DRIVERS_DIR}/pit/pit.cpp
    ${ARCH_DRIVERS_DIR}/apic/apic.cpp
    ${ARCH_DRIVERS_DIR}/serial/serial.cpp
    ${ARCH_DRIVERS_DIR}/keyboard/keyboard.cpp
)

# General purpose kernel library sources (not arch specific)
set(LIB_KERNEL_SOURCES
    ${LIB_DIR}/boot/limine-boot.cpp
    ${LIB_DIR}/tty/tty.cpp
    ${LIB_DIR}/shell/shell.cpp
    ${LIB_DIR}/timer/timer.cpp
    ${LIB_DIR}/panic/panic.cpp
    ${LIB_DIR}/memory/memory.cpp
    ${LIB_DIR}/memory/pmm.cpp
    ${LIB_DIR}/memory/slab.cpp
    ${LIB_DIR}/console/console.cpp
    ${LIB_DIR}/drivers/framebuffer/framebuffer.cpp
    ${LIB_DIR}/drivers/acpi/acpi.cpp
    ${LIB_DIR}/crt/crt.c
    ${LIB_DIR}/algo/algo.cpp
    ${LIB_DIR}/fs/vfs.cpp
    ${LIB_DIR}/fs/initramfs/initramfs.cpp
    ${LIB_DIR}/fs/initramfs/tar.cpp
)

# Test sources (compiled when KERNEL_TESTS is defined)
file(GLOB_RECURSE TEST_SOURCES ${TEST_DIR}/*.cpp)

# Architecture-specific boot files
set(BOOT_SOURCES
    ${ARCH_DIR}/boot/entry.cpp
)

# Architecture-specific CRT (C Runtime) files
set(CRT_SOURCES
    ${ARCH_DIR}/crt/crti.s
    ${ARCH_DIR}/crt/crtn.s
)

# Architecture-specific linker script
set(LINKER_SCRIPT ${ARCH_DIR}/limine.ld)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}           # Allows "arch/x86_64/..." includes
)

# Set ASM flags for 64-bit assembly
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --64")

# Create boot object library
add_library(boot_objs OBJECT ${BOOT_SOURCES})

# Create CRT object libraries
add_library(crti_obj OBJECT ${ARCH_DIR}/crt/crti.s)
add_library(crtn_obj OBJECT ${ARCH_DIR}/crt/crtn.s)

# Create kernel object library
add_library(kernel_objs OBJECT
  ${KERNEL_SOURCES}
  ${ARCH_DRIVER_KERNEL_SOURCES}
  ${ARCH_KERNEL_SOURCES}
  ${LIB_KERNEL_SOURCES}
  ${TEST_SOURCES}
)

# Apply freestanding flags to boot objects
target_compile_options(boot_objs PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-stack-protector>
)

# Kernel-specific compile options
target_compile_options(kernel_objs PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-stack-protector>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
)

# Enable kernel tests
target_compile_definitions(kernel_objs PRIVATE KERNEL_TESTS)

# Set C++ standard
set_target_properties(kernel_objs PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Print build information
message(STATUS "Kernel architecture: ${KERNEL_ARCH}")
message(STATUS "Linker script: ${LINKER_SCRIPT}")
message(STATUS "Boot sources: ${BOOT_SOURCES}")
