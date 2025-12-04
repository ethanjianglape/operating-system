#!/bin/bash

# Script to create a bootable ISO with GRUB

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check if kernel exists
if [ ! -f "build/kernel.elf" ]; then
    echo -e "${RED}Error: build/kernel.elf not found. Run ./build.sh first${NC}"
    exit 1
fi

# Check for grub-mkrescue
if ! command -v grub-mkrescue &> /dev/null; then
    echo -e "${RED}Error: grub-mkrescue not found${NC}"
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt install grub-pc-bin xorriso"
    echo "  Arch Linux: sudo pacman -S grub xorriso"
    echo "  Fedora: sudo dnf install grub2-tools-extra xorriso"
    exit 1
fi

echo -e "${YELLOW}Creating bootable ISO...${NC}"

# Create ISO directory structure
mkdir -p build/iso/boot/grub

# Copy kernel
cp build/kernel.elf build/iso/boot/kernel.elf

# Create GRUB configuration
cat > build/iso/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "MyOS" {
    multiboot2 /boot/kernel.elf
    boot
}
EOF

# Create ISO
grub-mkrescue -o build/myos.iso build/iso 2>&1 | grep -v "warning: 'xorriso'"

if [ -f "build/myos.iso" ]; then
    echo -e "${GREEN}ISO created successfully: build/myos.iso${NC}"
    echo ""
    echo "To run with QEMU:"
    echo "  qemu-system-x86_64 -cdrom build/myos.iso"
else
    echo -e "${RED}Failed to create ISO${NC}"
    exit 1
fi
