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

rm -rf isodir
rm -f ../myos.iso

mkdir -p isodir

echo "Copying sysroot to ISO directory..."
cp -R ../sysroot/* ./isodir/ 2>/dev/null || true

mkdir -p isodir/boot/limine
mkdir -p isodir/EFI/BOOT
cp /usr/share/limine/limine-bios.sys isodir/boot/limine
cp /usr/share/limine/limine-bios-cd.bin isodir/boot/limine
cp /usr/share/limine/limine-uefi-cd.bin isodir/boot/limine
cp /usr/share/limine/BOOTX64.EFI isodir/EFI/BOOT
cp ../src/bootloader/limine/limine.conf isodir/limine.conf

xorriso -as mkisofs \
        -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        -o ../myos.iso isodir/

limine bios-install ../myos.iso

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

