cmake_minimum_required(VERSION 3.16)

project(userspace C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directory for user binaries
set(USER_BIN_DIR "${CMAKE_SOURCE_DIR}/initramfs/bin")

# Linker script
set(USER_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/user.ld")

# Function to add a userspace program
function(add_user_program name source)
    add_executable(${name} ${source})

    # Remove inherited kernel flags and set userspace flags
    set_target_properties(${name} PROPERTIES
        COMPILE_OPTIONS ""
        RUNTIME_OUTPUT_DIRECTORY ${USER_BIN_DIR}
    )

    # Userspace compile flags (override any inherited flags)
    target_compile_options(${name} PRIVATE
        -m64
        -march=x86-64
        -ffreestanding
        -fno-stack-protector
        -fno-pic
        -nostdlib
        -mno-red-zone
        -Wall
        -Wextra
    )

    # Linker flags
    target_link_options(${name} PRIVATE
        -nostdlib
        -static
        -T ${USER_LINKER_SCRIPT}
    )
endfunction()

# Add userspace programs
add_user_program(hello hello.c)
