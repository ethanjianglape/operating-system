# MyOS

A hobby operating system for x86 (i386) architecture, written in C++23 and assembly.

## Features

**Current Status:**
- ✅ Higher-half kernel (3GB/1GB memory split)
- ✅ x86 protected mode with paging
- ✅ GDT with ring 0/3 support
- ✅ IDT with interrupt handling
- ✅ Syscall mechanism (int 0x80)
- ✅ TSS for privilege transitions
- ✅ VGA text mode (80x25)
- ✅ Colored kernel logging
- ✅ Basic kprintf() implementation
- 🚧 Process management (in progress)

**Memory Layout:**
```
0x00000000 - 0x02000000  (0-32MB)    Kernel low (hardware/MMIO)
0x02000000 - 0x04000000  (32-64MB)   Userspace
0xC0000000 - 0xD0000000  (3GB-3GB+256MB)  Kernel high
```

## Project Structure

```
os/
├── src/
│   ├── kernel/
│   │   ├── kernel.cpp              # Kernel entry point
│   │   ├── arch/i386/              # x86-specific code
│   │   │   ├── boot/
│   │   │   │   └── boot.s          # Bootloader, paging setup
│   │   │   ├── gdt/                # Global Descriptor Table
│   │   │   ├── idt/                # Interrupt Descriptor Table
│   │   │   ├── paging/             # Virtual memory management
│   │   │   ├── vga/                # VGA text mode driver
│   │   │   ├── syscall/            # System call handling
│   │   │   └── process/            # Process management
│   │   ├── lib/                    # Kernel libraries
│   │   │   ├── kprintf/            # Kernel printf
│   │   │   └── log/                # Colored logging
│   │   └── include/                # Kernel headers
│   └── libc/                       # C standard library (freestanding)
│       ├── stdio/
│       ├── string/
│       └── include/
├── sysroot/                        # System root (installed files)
└── CMakeLists.txt                  # Build configuration
```

## Building

**Requirements:**
- GCC/G++ with 32-bit multilib support
- CMake 3.16+
- GNU assembler
- GRUB (for bootable image)
- QEMU (for testing)

**Build Commands:**
```bash
./configure.sh
```

**Run in QEMU:**
```bash
qemu-system-x86_64 -cdrom myos.iso
```

## Architecture

**Boot Sequence:**
1. GRUB loads kernel at physical `0x00200000`
2. `boot.s` sets up GDT and enables paging
3. Jump to higher-half (`kernel_main` at `0xC0000000`)
4. Initialize subsystems (GDT, IDT, paging, etc.)
5. Enter kernel main loop

**Key Design Decisions:**
- **Higher-half kernel**: Kernel at 3GB+, userspace at 0-3GB
- **Function pointer abstraction**: Console/logging architecture-independent
- **Modern C++**: Using C++23 with freestanding implementation
- **Multiboot2**: Compatible with GRUB bootloader

## License

GNU General Public License v3.0

## Acknowledgments

Built following OS development resources:
- Intel Software Developer Manual
- OSDev Wiki
- Linux kernel source code (for reference)
