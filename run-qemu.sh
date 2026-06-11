#!/bin/bash

#BIOS="/usr/share/edk2/x64/OVMF.4m.fd"
BIOS="./OVMF.fd"

if [[ -f "$BIOS" ]]; then
    qemu-system-x86_64 -accel kvm -cpu host,+invtsc,migratable=no -bios ${BIOS} -serial stdio -m 512M  -cdrom myos.iso
else
    echo "Failed to find BIOS file at $BIOS"
    echo "edk2-ovmf must be installed to run qemu in UEFI mode"
    echo "See for more info: https://github.com/tianocore/edk2"
fi
