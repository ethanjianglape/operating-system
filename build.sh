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

cd build

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
echo -e "${YELLOW}Building kernel...${NC}"
make -j$(nproc)

# Check if build was successful
if [ -f "kernel.elf" ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo -e "${GREEN}Kernel binary: build/kernel.elf${NC}"
    echo -e "${GREEN}Raw binary: build/kernel.bin${NC}"
    echo ""

    # Create bootable ISO
    echo -e "${YELLOW}Creating bootable ISO...${NC}"

    # Check for grub-mkrescue
    if command -v grub-mkrescue &> /dev/null; then
        # Create ISO directory structure
        mkdir -p iso/boot/grub

        # Copy kernel
        cp kernel.elf iso/boot/kernel.elf

        # Create GRUB configuration
        cat > iso/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "MyOS" {
    set gfxpayload=text
    multiboot2 /boot/kernel.elf
    boot
}
EOF

        # Create ISO
        grub-mkrescue -o myos.iso iso 2>&1 | grep -v "warning: 'xorriso'" || true

        if [ -f "myos.iso" ]; then
            echo -e "${GREEN}ISO created: build/myos.iso${NC}"
            echo ""
            echo "To run with QEMU:"
            echo "  qemu-system-x86_64 -cdrom build/myos.iso"
        else
            echo -e "${YELLOW}Warning: Failed to create ISO${NC}"
            echo "You can still run the kernel directly (may show warnings on newer QEMU):"
            echo "  qemu-system-x86_64 -kernel build/kernel.elf"
        fi
    else
        echo -e "${YELLOW}Warning: grub-mkrescue not found. Skipping ISO creation.${NC}"
        echo "Install it with:"
        echo "  Ubuntu/Debian: sudo apt install grub-pc-bin xorriso"
        echo "  Arch Linux: sudo pacman -S grub xorriso"
        echo ""
        echo "To run kernel directly (may show warnings on newer QEMU):"
        echo "  qemu-system-x86_64 -kernel build/kernel.elf"
    fi
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
