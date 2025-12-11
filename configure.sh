#!/bin/bash

# Simple build script for MyOS kernel

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building MyOS Kernel...${NC}"

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

if [ ! -d "sysroot" ]; then
    echo "Creating sysroot directory..."
    mkdir sysroot
fi

cd build

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_ASM_COMPILER=as -DCMAKE_BUILD_TYPE=Debug

# Build (CMakeLists.txt handles cleaning and copying to sysroot)
echo -e "${YELLOW}Building kernel...${NC}"
make -j$(nproc)

# Check if build was successful
if [ ! -f "../sysroot/boot/kernel.elf" ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"
echo -e "${GREEN}Kernel binary: sysroot/boot/kernel.elf${NC}"
echo -e "${GREEN}Raw binary: sysroot/boot/kernel.bin${NC}"
echo ""

# Create bootable ISO using sysroot
echo -e "${YELLOW}Creating bootable ISO from sysroot...${NC}"

# Check for grub-mkrescue
if command -v grub-mkrescue &> /dev/null; then
    # Clean up any previous ISO build artifacts
    rm -rf isodir
    rm -f ../myos.iso

    # Create ISO directory structure using sysroot as base
    mkdir -p isodir

    # Copy entire sysroot to isodir
    echo "Copying sysroot to ISO directory..."
    cp -R ../sysroot/* isodir/ 2>/dev/null || true

    # Create GRUB directory and copy configuration
    mkdir -p isodir/boot/grub
    cp ../src/grub/grub.cfg isodir/boot/grub/grub.cfg

    # Create ISO in project root
    grub-mkrescue -o ../myos.iso isodir 2>&1 | grep -v "warning: 'xorriso'" || true

    if [ -f "../myos.iso" ]; then
        echo -e "${GREEN}ISO created: myos.iso${NC}"
        echo ""
        echo "To run with QEMU:"
        echo "  qemu-system-x86_64 -cdrom myos.iso"
    else
        echo -e "${YELLOW}Warning: Failed to create ISO${NC}"
        echo "You can still run the kernel directly:"
        echo "  qemu-system-x86_64 -kernel ../sysroot/boot/kernel.elf"
    fi

    # Clean up ISO directory
    rm -rf isodir
else
    echo -e "${YELLOW}Warning: grub-mkrescue not found. Skipping ISO creation.${NC}"
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt install grub-pc-bin xorriso"
    echo "  Arch Linux: sudo pacman -S grub xorriso"
    echo ""
    echo "To run kernel directly:"
    echo "  qemu-system-x86_64 -kernel ../sysroot/boot/kernel.elf"
fi
