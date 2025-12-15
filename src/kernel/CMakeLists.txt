cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(kernel C CXX ASM)

# Architecture-specific settings
set(KERNEL_ARCH "i686" CACHE STRING "Target architecture")
set(ARCH_DIR "arch/${KERNEL_ARCH}")
set(ARCH_DRIVERS_DIR "arch/${KERNEL_ARCH}/drivers")
set(LIB_DIR "lib/")

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
    ${ARCH_DIR}/timers/timer.cpp
    ${ARCH_DIR}/syscall/syscall.cpp
    ${ARCH_DIR}/syscall/syscall_entry.s
    ${ARCH_DIR}/vmm/vmm.cpp
    ${ARCH_DIR}/process/process.cpp
    ${ARCH_DIR}/process/process.s
)

set(ARCH_DRIVER_KERNEL_SOURCES
    ${ARCH_DRIVERS_DIR}/vga/vga.cpp
    ${ARCH_DRIVERS_DIR}/pic/pic.cpp
    ${ARCH_DRIVERS_DIR}/pit/pit.cpp
    ${ARCH_DRIVERS_DIR}/apic/apic.cpp
)

# General purpose kernel library sources (not arch specific)
set(LIB_KERNEL_SOURCES
    ${LIB_DIR}/kprintf/kprintf.cpp
    ${LIB_DIR}/log/log.cpp
    ${LIB_DIR}/panic/panic.cpp
    ${LIB_DIR}/memory/memory.cpp
    ${LIB_DIR}/memory/pmm.cpp
)

# Architecture-specific boot files
set(BOOT_SOURCES
    ${ARCH_DIR}/boot/boot.s
)

# Architecture-specific CRT (C Runtime) files
set(CRT_SOURCES
    ${ARCH_DIR}/crt/crti.s
    ${ARCH_DIR}/crt/crtn.s
)

# Architecture-specific linker script
set(LINKER_SCRIPT ${ARCH_DIR}/linker.ld)

# Note: Other files in arch/i686/boot (boot.asm, long_mode_start.asm) are kept for reference
# but not included in the build

# Kernel headers - install to sysroot at configure time
file(MAKE_DIRECTORY ${SYSROOT}/usr/include/kernel)
file(GLOB_RECURSE KERNEL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/kernel/*.h")
foreach(HEADER ${KERNEL_HEADERS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/kernel" ${HEADER})
    get_filename_component(HEADER_DIR ${REL_PATH} DIRECTORY)
    if(HEADER_DIR)
        file(MAKE_DIRECTORY ${SYSROOT}/usr/include/kernel/${HEADER_DIR})
    endif()
    configure_file(${HEADER} ${SYSROOT}/usr/include/kernel/${REL_PATH} COPYONLY)
endforeach()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}           # Allows "arch/i686/..." includes
    ${SYSROOT}/usr/include
    ${CMAKE_SOURCE_DIR}/include
)

# Set ASM flags for 32-bit assembly
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --32")

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
message(STATUS "CRT sources: ${CRT_SOURCES}")
message(STATUS "Kernel sources: ${KERNEL_SOURCES}")
message(STATUS "Arch-specific kernel sources: ${ARCH_KERNEL_SOURCES}")
message(STATUS "Kernel headers installed to: ${SYSROOT}/usr/include/kernel")
