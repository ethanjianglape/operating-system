cmake_minimum_required(VERSION 3.16)

project(libc C ASM)

# Configuration variables
set(HOSTARCH "i686" CACHE STRING "Target architecture")
set(SYSROOT "${CMAKE_SOURCE_DIR}/sysroot" CACHE PATH "Sysroot directory")

# Install headers to sysroot FIRST (before building anything)
file(MAKE_DIRECTORY ${SYSROOT}/usr/include)
file(GLOB_RECURSE LIBC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
foreach(HEADER ${LIBC_HEADERS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include" ${HEADER})
    get_filename_component(HEADER_DIR ${REL_PATH} DIRECTORY)
    file(MAKE_DIRECTORY ${SYSROOT}/usr/include/${HEADER_DIR})
    configure_file(${HEADER} ${SYSROOT}/usr/include/${REL_PATH} COPYONLY)
endforeach()

# Include directories (use sysroot)
include_directories(
    ${SYSROOT}/usr/include
)

# Compiler flags for freestanding libc
set(LIBC_CFLAGS
    -ffreestanding
    -Wall
    -Wextra
    -fno-stack-protector
    -fno-pic
    -mno-80387
    -mno-mmx
    -mno-sse
    -mno-sse2
)

# Additional flags for libk (kernel version)
set(LIBK_CFLAGS ${LIBC_CFLAGS})
set(LIBK_CPPFLAGS -D__is_libk)

# Freestanding objects (work in both kernel and userspace)
set(FREESTANDING_SOURCES
    stdio/printf.c
    stdio/putchar.c
    stdio/puts.c
    string/memcmp.c
    string/memcpy.c
    string/memmove.c
    string/memset.c
    string/strlen.c
)

# Architecture-specific sources (currently empty for i686)
set(ARCH_FREESTANDING_SOURCES
    # Add arch-specific sources here if needed
)

# All sources for the library
set(ALL_SOURCES
    ${FREESTANDING_SOURCES}
    ${ARCH_FREESTANDING_SOURCES}
)

# Build libk.a (kernel version of libc)
add_library(libk STATIC ${ALL_SOURCES})

# Set target properties for libk
target_compile_options(libk PRIVATE ${LIBK_CFLAGS})
target_compile_definitions(libk PRIVATE ${LIBK_CPPFLAGS})
set_target_properties(libk PROPERTIES
    OUTPUT_NAME "k"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libc
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS ON  # Use gnu11 for compatibility
)

# Copy libk.a to sysroot after building
file(MAKE_DIRECTORY ${SYSROOT}/usr/lib)
add_custom_command(TARGET libk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:libk>
        ${SYSROOT}/usr/lib/libk.a
    COMMENT "Copying libk.a to sysroot/usr/lib"
)

# Optional: Build libc.a (userspace version) - currently not fully implemented
# Uncomment when ready for hosted environment
# add_library(libc STATIC ${ALL_SOURCES})
# target_compile_options(libc PRIVATE ${LIBC_CFLAGS})
# target_compile_definitions(libc PRIVATE -D__is_libc)
# set_target_properties(libc PROPERTIES
#     OUTPUT_NAME "c"
#     ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libc
#     C_STANDARD 11
#     C_STANDARD_REQUIRED ON
#     C_EXTENSIONS ON
# )

# Installation rules - install libk to sysroot
install(TARGETS libk
    ARCHIVE DESTINATION ${SYSROOT}/usr/lib
)

# Print build information
message(STATUS "Building libc for architecture: ${HOSTARCH}")
message(STATUS "Libc headers installed to: ${SYSROOT}/usr/include")
message(STATUS "libk will be installed to: ${SYSROOT}/usr/lib")
