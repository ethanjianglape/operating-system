#!/bin/bash

# Simple build script for MyOS kernel

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_section() {
    CURRENT_TIME=$(date +"%T.%3N")
    echo -e "${YELLOW}[$CURRENT_TIME] $1${NC}"
}

log_failure() {
    CURRENT_TIME=$(date +"%T.%3N")
    echo -e "${RED}[$CURRENT_TIME] FAILURE! $1${NC}"
}

log_success() {
    CURRENT_TIME=$(date +"%T.%3N")
    echo -e "${GREEN}[$CURRENT_TIME] $1${NC}"
}

log() {
    CURRENT_TIME=$(date +"%T.%3N")
    echo -e "[$CURRENT_TIME] $1${NC}"
}

log_section "Building MyOS Kernel"
log_section "Creating build directories"

# Create build directory if it doesn't exist
log "Creating build directory..."
mkdir -p cmake_build

log "Creating sysroot directory..."
mkdir -p sysroot

log "Creating initframfs directory..."
mkdir -p initramfs/bin

cd cmake_build

# Configure with CMake
log_section "Configuring CMake"
cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_ASM_COMPILER=as -DCMAKE_BUILD_TYPE=Debug

# Build (CMakeLists.txt handles cleaning and copying to sysroot)
log_section "Rebuilding kernel"
make clean
bear -- make -j$(nproc)

if [ -f "./compile_commands.json" ]; then
    log_section "Copying compile_commands.json"
    cp ./compile_commands.json ../
fi

# Check if build was successful
if [ ! -f "../sysroot/boot/kernel.elf" ]; then
    log_failure "Build failed! sysroot/boot/kernel.elf not found!"
    exit 1
fi

if [ -d "../initramfs" ]; then
    log_section "Creating initramfs.tar"
    tar -cvf ../sysroot/boot/initramfs.tar -C ../initramfs .
fi

log_success "Build successful!"
log_success "Kernel binary: sysroot/boot/kernel.elf"
log_success "Raw binary: sysroot/boot/kernel.bin"

# Create bootable ISO using sysroot
log_section "Creating bootable ISO from sysroot"

log "Cleaning up old iso files..."
rm -rf isodir
rm -f ../myos.iso
mkdir -p isodir

log "Copying sysroot to ISO directory..."
cp -R ../sysroot/* ./isodir/ 2>/dev/null || true

log_section "Creating bootloader and UEFI dependencies"
mkdir -p isodir/boot/limine
mkdir -p isodir/EFI/BOOT

log_section "Copying limine BIOS and UEFI system files"

if [ -f "/usr/local/share/limine/limine-bios.sys" ]; then
    cp /usr/local/share/limine/limine-bios.sys isodir/boot/limine
elif [ -f "/usr/share/limine/limine-bios.sys" ]; then
    cp /usr/share/limine/limine-bios.sys isodir/boot/limine
else
    log_failure "Could not find limine-bios.sys!"
    exit 1
fi

cp /usr/local/share/limine/limine-bios-cd.bin isodir/boot/limine
cp /usr/local/share/limine/limine-uefi-cd.bin isodir/boot/limine
cp /usr/local/share/limine/BOOTX64.EFI isodir/EFI/BOOT
cp ../src/bootloader/limine/limine.conf isodir/limine.conf

log_section "Using xorriso to build bootable ISO"
xorriso -as mkisofs \
        -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        -o ../myos.iso isodir/

log_section "Installing limine bios into ISO"
limine bios-install ../myos.iso

echo ""

if [ -f "../myos.iso" ]; then
    log_success "MyOS Build Successful!"
    log_success "ISO Created: myos.iso"
else
    log_failure "Failed create ISO!"
fi
