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
  ${ARCH_DIR}/cpu/cpu.cpp
  ${ARCH_DIR}/gdt/gdt.cpp
  ${ARCH_DIR}/gdt/gdt.s
  ${ARCH_DIR}/interrupts/idt.cpp
  ${ARCH_DIR}/interrupts/irq.cpp
  ${ARCH_DIR}/interrupts/isr.s
  ${ARCH_DIR}/entry/entry.cpp
  ${ARCH_DIR}/entry/syscall_entry.s
  ${ARCH_DIR}/percpu/percpu.cpp
  ${ARCH_DIR}/memory/vmm.cpp
  ${ARCH_DIR}/context/context_switch.s
)

set(ARCH_DRIVER_KERNEL_SOURCES
  ${ARCH_DRIVERS_DIR}/pic/pic.cpp
  ${ARCH_DRIVERS_DIR}/pit/pit.cpp
  ${ARCH_DRIVERS_DIR}/apic/apic.cpp
  ${ARCH_DRIVERS_DIR}/serial/serial.cpp
  ${ARCH_DRIVERS_DIR}/keyboard/keyboard.cpp
  ${ARCH_DRIVERS_DIR}/keyboard/ps2.cpp
  ${ARCH_DRIVERS_DIR}/tsc/tsc.cpp
)

# General purpose kernel library sources (not arch specific)
set(LIB_KERNEL_SOURCES
  ${LIB_DIR}/boot/limine-boot.cpp
  ${LIB_DIR}/timer/timer.cpp
  ${LIB_DIR}/kpanic/kpanic.cpp
  ${LIB_DIR}/memory/memory.cpp
  ${LIB_DIR}/memory/pmm.cpp
  ${LIB_DIR}/memory/slab.cpp
  ${LIB_DIR}/memory/new.cpp
  ${LIB_DIR}/console/console.cpp
  ${LIB_DIR}/console/ansi.cpp
  ${LIB_DIR}/framebuffer/framebuffer.cpp
  ${LIB_DIR}/acpi/acpi.cpp
  ${LIB_DIR}/acpi/madt.cpp
  ${LIB_DIR}/crt/crt.c
  ${LIB_DIR}/algo/algo.cpp
  ${LIB_DIR}/fs/fs.cpp
  ${LIB_DIR}/fs/fs_file_ops.cpp
  ${LIB_DIR}/fs/initramfs/initramfs.cpp
  ${LIB_DIR}/fs/initramfs/tar.cpp
  ${LIB_DIR}/fs/devfs/devfs.cpp
  ${LIB_DIR}/fs/devfs/dev_tty.cpp
  ${LIB_DIR}/fs/devfs/dev_null.cpp
  ${LIB_DIR}/fs/devfs/dev_random.cpp
  ${LIB_DIR}/process/elf.cpp
  ${LIB_DIR}/process/process.cpp
  ${LIB_DIR}/syscall/sys_fd.cpp
  ${LIB_DIR}/syscall/sys_sleep.cpp
  ${LIB_DIR}/syscall/sys_proc.cpp
  ${LIB_DIR}/scheduler/scheduler.cpp
  ${LIB_DIR}/acpica/acpica_glue.cpp
)

# Test sources (compiled when KERNEL_TESTS is defined)
file(GLOB_RECURSE TEST_SOURCES ${TEST_DIR}/*.cpp)

# Architecture-specific boot files
set(BOOT_SOURCES
  ${ARCH_DIR}/boot/limine_entry.cpp
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
option(KERNEL_TESTS_QUIET "Suppress passing test output" ON)
target_compile_definitions(kernel_objs PRIVATE KERNEL_TESTS)
if(KERNEL_TESTS_QUIET)
  target_compile_definitions(kernel_objs PRIVATE KERNEL_TESTS_QUIET)
endif()

# Enable debug assertions (bounds checking, etc.)
option(KERNEL_DEBUG "Enable debug assertions" OFF)
if(KERNEL_DEBUG)
  target_compile_definitions(kernel_objs PRIVATE KERNEL_DEBUG)
  message(STATUS "Debug assertions: ENABLED")
endif()

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

# =============================================================================
# ACPICA Integration
# =============================================================================

set(ACPICA_DIR "${CMAKE_SOURCE_DIR}/acpica/source")

# ACPICA component sources (exclude debugger and disassembler)
file(GLOB ACPICA_DISPATCHER_SOURCES "${ACPICA_DIR}/components/dispatcher/*.c")
file(GLOB ACPICA_EVENTS_SOURCES "${ACPICA_DIR}/components/events/*.c")
file(GLOB ACPICA_EXECUTER_SOURCES "${ACPICA_DIR}/components/executer/*.c")
file(GLOB ACPICA_HARDWARE_SOURCES "${ACPICA_DIR}/components/hardware/*.c")
file(GLOB ACPICA_NAMESPACE_SOURCES "${ACPICA_DIR}/components/namespace/*.c")
file(GLOB ACPICA_PARSER_SOURCES "${ACPICA_DIR}/components/parser/*.c")
file(GLOB ACPICA_RESOURCES_SOURCES "${ACPICA_DIR}/components/resources/*.c")
file(GLOB ACPICA_TABLES_SOURCES "${ACPICA_DIR}/components/tables/*.c")
file(GLOB ACPICA_UTILITIES_SOURCES "${ACPICA_DIR}/components/utilities/*.c")

set(ACPICA_SOURCES
    ${ACPICA_DISPATCHER_SOURCES}
    ${ACPICA_EVENTS_SOURCES}
    ${ACPICA_EXECUTER_SOURCES}
    ${ACPICA_HARDWARE_SOURCES}
    ${ACPICA_NAMESPACE_SOURCES}
    ${ACPICA_PARSER_SOURCES}
    ${ACPICA_RESOURCES_SOURCES}
    ${ACPICA_TABLES_SOURCES}
    ${ACPICA_UTILITIES_SOURCES}
)

# Exclude debugger-only dump files
list(FILTER ACPICA_SOURCES EXCLUDE REGEX ".*dump.*\\.c$")

# OSL implementation
set(ACPICA_OSL_SOURCES
    ${LIB_DIR}/acpica/acpica_osl.c
)

# Create ACPICA object library with C-specific flags
add_library(acpica_objs OBJECT ${ACPICA_SOURCES} ${ACPICA_OSL_SOURCES})

# ACPICA compile options (C99, kernel flags, platform define)
# Undefine __linux__ so ACPICA uses our acmyos.h platform header instead of aclinux.h.
# We still use system headers but provide glibc ctype stubs in crt.c.
target_compile_options(acpica_objs PRIVATE
    -std=c99
    -m64
    -march=x86-64
    -mcmodel=kernel
    -mno-red-zone
    -ffreestanding
    -fno-stack-protector
    -fno-pic
    -mno-80387
    -mno-mmx
    -mno-sse
    -mno-sse2
    -g
    -O1
    -U__linux__
    -U_LINUX
    -D__MYOS__
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

# ACPICA include directories
target_include_directories(acpica_objs PRIVATE
    ${ACPICA_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/acpica
)

# Add ACPICA headers to kernel_objs so kernel code can include acpi.h
target_include_directories(kernel_objs PRIVATE
    ${ACPICA_DIR}/include
)

message(STATUS "ACPICA sources: ${ACPICA_DIR}/components/")
