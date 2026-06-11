cmake_minimum_required(VERSION 3.16)

project(userspace C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directory for user binaries
set(USER_BIN_DIR "${CMAKE_SOURCE_DIR}/initramfs/bin")

# Linker script
set(USER_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/user.ld")

set(MUSL_GCC_FLAGS "-static")

include(CheckCCompilerFlag)
check_c_compiler_flag(-fno-link-libatomic HAS_NO_LINK_LIBATOMIC)

if (HAS_NO_LINK_LIBATOMIC)
  set(MUSL_GCC_FLAGS "-fno-link-libatomic ${MUSL_GCC_FLAGS}")
endif()

# Function to add a musl libc program (compiled with musl-gcc, statically linked)
function(add_musl_program name source)
    add_custom_command(
        OUTPUT ${USER_BIN_DIR}/${name}
        COMMAND musl-gcc ${MUSL_GCC_FLAGS} -o ${USER_BIN_DIR}/${name} ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        COMMENT "Building musl program: ${name}"
    )
    add_custom_target(${name} ALL DEPENDS ${USER_BIN_DIR}/${name})
endfunction()

# Add userspace programs
add_musl_program(shell shell.c)
add_musl_program(musl musl.c)
