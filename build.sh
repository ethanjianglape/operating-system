#!/bin/bash

# Simple build script for MyOS kernel

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_section() {
    echo -e "${YELLOW}$1${NC}"
}

log_failure() {
    echo -e "${RED}FAILURE! $1${NC}"
}

log_success() {
    echo -e "${GREEN}$1${NC}"
}

log() {
    echo -e "$1${NC}"
}

log_section "Building MyOS"

mkdir -p cmake_build sysroot initramfs/bin

cd cmake_build

log_section "Configuring CMake"
cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_ASM_COMPILER=as -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

rm -f kernel.elf
make -j$(nproc)

if [ -f "./compile_commands.json" ]; then
    log_section "Copying compile_commands.json"
    cp ./compile_commands.json ../
fi

log_success "Build successful!"
log_success "Kernel: sysroot/boot/kernel.elf"
log_success "ISO: myos.iso"
