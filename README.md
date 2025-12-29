# MyOS

A hobby operating system for x86_64 architecture, written in C++23 and assembly.

## Disclaimer

**This is a hobby operating system project for educational purposes.**

- ⚠️ Not production-ready or security-hardened
- ⚠️ May contain bugs or undefined behavior
- ⚠️ **Not recommended for use on real hardware**
- ✅ Safe for use in virtual machines (QEMU, VirtualBox, etc.)

Use at your own risk. The author(s) are not responsible for any damage, data loss, or hardware issues that may result from running this software.

## Target Architecture

**Target:** x86_64 (AMD64/Intel 64)
- Requires APIC support (LAPIC + IOAPIC)
- Uses Limine bootloader protocol
- Tested on: QEMU

## Features

**Current Status:**
- ✅ Higher-half kernel with HHDM (Higher Half Direct Map)
- ✅ Limine bootloader with framebuffer
- ✅ Physical memory manager (PMM) with bitmap allocator
- ✅ Virtual memory manager (VMM) with 4-level paging
- ✅ Slab allocator for efficient small object allocation (32-1024 bytes)
- ✅ GDT with ring 0/3 support
- ✅ IDT with interrupt handling
- ✅ ACPI table parsing (RSDP, XSDT, MADT)
- ✅ APIC support (LAPIC + IOAPIC)
- ✅ PS/2 keyboard driver with scancode translation
- ✅ Framebuffer console with PSF font rendering
- ✅ Serial output (COM1) for kernel logging
- ✅ Dynamic containers (kstring, kvector)
- ✅ Interactive shell with line editing
- ✅ Syscall mechanism

**Planned:**
- 🚧 VFS and initramfs
- 🚧 Process management

## Project Structure

```
os/
├── src/kernel/
│   ├── kernel.cpp                  # Kernel entry point
│   ├── include/                    # Kernel headers (flat structure)
│   │   ├── arch.hpp                # Architecture abstraction
│   │   ├── containers/             # kstring, kvector
│   │   ├── memory/                 # kmalloc, PMM, Slab allocator
│   │   ├── log/                    # Logging utilities
│   │   ├── kprint/                 # Serial output
│   │   └── ...
│   ├── lib/                        # Implementations (mirrors include/)
│   │   ├── containers/
│   │   ├── memory/                 # kmalloc, PMM, Slab allocator
│   │   ├── shell/                  # Interactive shell, command parsing
│   │   ├── tty/                    # Terminal handling, line editing
│   │   └── ...
│   ├── arch/x86_64/                # x86_64-specific (headers + source together)
│   │   ├── boot/                   # Limine entry, early init
│   │   ├── gdt/                    # Global Descriptor Table
│   │   ├── interrupts/             # IDT, IRQ handling
│   │   ├── vmm/                    # Virtual memory manager
│   │   ├── drivers/
│   │   │   ├── apic/               # LAPIC + IOAPIC
│   │   │   ├── keyboard/           # PS/2 keyboard
│   │   │   ├── serial/             # COM1 serial
│   │   │   └── ...
│   │   └── ...
│   └── CONVENTIONS.md              # Code style and structure guide
├── limine/                         # Limine bootloader files
└── configure.sh                    # Build script
```

See `src/kernel/CONVENTIONS.md` for namespace and code style conventions.

## Building

**Requirements:**
- GCC/G++ cross-compiler for x86_64-elf (or system compiler with proper flags)
- CMake 3.16+
- GNU assembler
- Limine bootloader
- xorriso (for ISO creation)
- QEMU (for testing)

**Build Commands:**
```bash
./configure.sh
```

**Run in QEMU:**
```bash
qemu-system-x86_64 -cdrom myos.iso
```

**With serial output (recommended for debugging):**
```bash
qemu-system-x86_64 -cdrom myos.iso -serial stdio
```

## Architecture

**Boot Sequence:**
1. Limine loads kernel and provides framebuffer, memory map, RSDP
2. Early init sets up GDT, IDT, PMM, VMM with HHDM
3. Parse ACPI tables (MADT) for APIC configuration
4. Initialize LAPIC and IOAPIC for interrupt routing
5. Initialize drivers (keyboard, serial)
6. Start interactive shell

**Key Design Decisions:**
- **Higher-half kernel**: Kernel mapped at high addresses via HHDM
- **Architecture abstraction**: `lib/` code uses `arch::` namespace, not `x86_64::` directly
- **Flat namespaces**: No `kernel::` prefix; subsystems use flat namespaces (`pmm::`, `log::`, `tty::`)
- **k-prefixed utilities**: Global types like `kstring`, `kvector`, `kprint()`
- **Modern C++**: C++23 with freestanding implementation, concepts, fold expressions

## License

GNU General Public License v3.0

## Acknowledgments

Built following OS development resources:
- Intel Software Developer Manual
- OSDev Wiki
- Limine bootloader documentation
