#!/bin/bash

BIOS="/usr/share/edk2/x64/OVMF.4m.fd"

if [[ -f "$BIOS" ]]; then
    qemu-system-x86_64 -bios /usr/share/edk2/x64/OVMF.4m.fd -serial stdio -m 512M -cdrom myos.iso
else
    echo "Failed to find BIOS file at $BIOS"
    echo "edk2-ovmf must be installed to run qemu in UEFI mode"
    echo "To install on"
    echo "    Arch Linux: pacman -S edk2-ovmf"
    echo "    Debian: apt install edk2"
fi
