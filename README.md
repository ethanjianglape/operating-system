# MyOS - Learning Operating System Kernel

A basic operating system kernel written in C++ for educational purposes.

## Prerequisites

- GCC cross-compiler (or native GCC for x86_64)
- NASM (Netwide Assembler)
- CMake (>= 3.16)
- QEMU (for testing)
- GNU Make
- objcopy (binutils)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt install build-essential cmake nasm qemu-system-x86 binutils
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake nasm qemu binutils
```

**Fedora:**
```bash
sudo dnf install gcc gcc-c++ cmake nasm qemu binutils
```

## Building

1. Create a build directory:
```bash
mkdir build
cd build
```

2. Configure with CMake (optionally using the toolchain file):
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-x86_64.cmake
```

Or simply:
```bash
cmake ..
```

3. Build the kernel:
```bash
make
```

This will produce:
- `kernel.elf` - The kernel executable in ELF format
- `kernel.bin` - The raw kernel binary
- `kernel.map` - Memory map file

## Running

### With QEMU

Run the kernel directly with QEMU:
```bash
qemu-system-x86_64 -kernel kernel.elf
```

Or with more verbose output:
```bash
qemu-system-x86_64 -kernel kernel.elf -serial stdio
```

### Creating a bootable ISO (Optional)

To create a bootable ISO with GRUB, you'll need `grub-mkrescue` and `xorriso`.

## Project Structure

```
.
├── CMakeLists.txt          # Main build configuration
├── linker.ld               # Linker script for memory layout
├── cmake/
│   └── toolchain-x86_64.cmake  # Cross-compilation toolchain
├── src/
│   ├── boot/
│   │   └── boot.asm        # Boot code with Multiboot header
│   └── kernel/
│       └── kernel.cpp      # Kernel entry point
└── include/                # Header files (add as needed)
```

## Features

- Multiboot2 compliant (bootable with GRUB)
- VGA text mode output
- Basic kernel initialization
- Freestanding C++ environment

## Next Steps

Some ideas for extending this kernel:

1. Add interrupt handling (IDT, PIC, ISRs)
2. Implement keyboard driver
3. Add memory management (paging, heap allocation)
4. Create a simple shell
5. Add task switching and scheduling
6. Implement file system support
7. Add user mode support

## Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [Intel Software Developer Manuals](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- [Multiboot Specification](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)

## License

This is a learning project. Feel free to use and modify as needed.
