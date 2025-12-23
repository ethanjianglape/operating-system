# Future Improvements

## Index

- [x] [Logging and Console Separation](#logging-and-console-separation)
- [ ] [Dynamic Collections (kvector/kstring)](#dynamic-collections)
- [ ] [VFS and Initramfs](#vfs-and-initramfs)
- [ ] [IOAPIC GSI Mapping](#ioapic-gsi-mapping)
- [ ] [PS/2 Controller Initialization](#ps2-controller-initialization)
- [ ] [USB HID Keyboard Support](#usb-hid-keyboard-support)

---

## Logging and Console Separation

**Status:** Complete

**Current state:** kprintf outputs directly to serial. Console/TTY stack handles user-facing display.

**Target architecture:**
```
Debug path:   kprintf() → serial only (unlimited, for kernel devs)
User path:    TTY → console → framebuffer (clean, user-facing)
```

**Implementation:**
- kprintf/kernel::log writes directly to serial output
- kernel::console becomes purely for TTY/user interaction
- TTY handles keyboard input, input buffer, line editing
- Console handles character rendering, cursor, scrolling
- Future: Add log levels to optionally route important messages to console

**Benefits:**
- No debug spew corrupting user's input line
- Unlimited serial logging without framebuffer scroll concerns
- Easier debugging via QEMU serial capture
- Clean separation of concerns

---

## Dynamic Collections

**Status:** Next up

**Prerequisite:** kfree implementation (VMM header-based size tracking, PMM free_frames)

**Why needed:** Shell argument parsing, path handling, VFS entries, command history all benefit from dynamic containers. Working without them is increasingly painful.

**Components:**
- `kvector<T>` - dynamic array with push_back, pop_back, operator[], iterators
- `kstring` - dynamic string with concatenation, c_str(), comparison

**Location:** `kernel/lib/containers/` or `kernel/lib/kstring.hpp`, `kernel/lib/kvector.hpp`

**Implementation notes:**
- Use kmalloc/kfree internally
- Growth factor of 2x on reallocation
- No exceptions - panic on allocation failure
- Keep interface minimal, add methods as needed

**Unlocks:**
- `kvector<kstring> args` for shell argument parsing
- Path manipulation with string concatenation
- VFS directory listings
- Command history

---

## VFS and Initramfs

**Status:** Not started

**Prerequisite:** Dynamic collections (kvector/kstring)

**Goal:** Enable shell commands like `ls` and `cat` to interact with files.

**Architecture:**
```
User/Shell: fs::open(), fs::read(), fs::readdir()
                    │
                    ▼
                   VFS (uniform interface)
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
    initramfs     FAT32      ISO9660
    (memory)      (disk)       (CD)
```

**Initial implementation (initramfs):**
- Load tar archive as Limine module
- Parse tar headers (512-byte blocks, simple format)
- Mount as root filesystem in memory
- Read-only is fine initially

**VFS interface:**
```cpp
namespace kernel::fs {
    struct DirEntry { kstring name; bool is_dir; size_t size; };

    int open(const char* path);
    ssize_t read(int fd, void* buf, size_t count);
    kvector<DirEntry> readdir(const char* path);
    void close(int fd);
}
```

**Shell commands this enables:**
- `ls [path]` - list directory contents
- `cat <file>` - print file contents
- `pwd` / `cd` - working directory (once state is added)

**Future backends:**
- FAT32 for USB drives / SD cards
- ext2 for more Unix-like semantics
- ISO9660 for CD-ROM images

---

## IOAPIC GSI Mapping

**Status:** Not started

**Current state:** Hardcoded assumptions for interrupt routing:
- GSI base = 0
- No IRQ overrides (IRQ N = GSI N = IOAPIC pin N)
- ISA defaults (edge triggered, active high)

**Real hardware note:** Keyboard works in QEMU but not on real laptop hardware. Likely cause is different GSI mappings or interrupt source overrides that differ from QEMU's defaults.

**Correct implementation:**
- Parse and store MADT Interrupt Source Overrides (currently logged but not stored)
- Lookup function to resolve IRQ → GSI with override check
- Find correct IOAPIC by GSI range (for multi-IOAPIC systems)
- Apply polarity/trigger flags from overrides when programming redirection entries

**Diagnosis steps:**
- Log 8042 status register on real hardware to verify controller presence
- Log parsed MADT overrides to compare QEMU vs real hardware
- Check if IRQ1 has an override on real hardware

---

## PS/2 Controller Initialization

**Status:** Not started

**Current state:** Assumes PS/2 controller and keyboard are present and working. Directly reads scancodes from port 0x60. Init sequence is commented out.

**Modern hardware considerations:**
- Many modern laptops lack a real 8042 PS/2 controller
- Internal laptop keyboards often connect via I2C/SMBus, not PS/2
- USB keyboards may work via BIOS "USB Legacy Support" emulation, but this often stops working once the OS takes over USB
- Reading 0xFF from port 0x64 typically means no controller present

**Correct implementation:**
- Check if 8042 controller exists before attempting init
- Disable both PS/2 ports during initialization
- Flush the output buffer (read port 0x60 until status bit 0 is clear)
- Read and modify controller configuration byte
- Perform controller self-test (command 0xAA, expect 0x55)
- Test first PS/2 port (command 0xAB, expect 0x00)
- Test second PS/2 port if present (command 0xA9)
- Enable ports and enable interrupts in config byte
- Reset keyboard device (send 0xFF, expect 0xFA ACK)
- Set scancode set if needed (default is usually fine)
- Handle initialization failures gracefully (fall back to USB HID if available)

---

## USB HID Keyboard Support

**Status:** Not started (long-term goal)

**Why needed:** PS/2 is increasingly unavailable on modern hardware. USB HID is the universal standard for keyboards.

**Required components:**
- XHCI (USB 3.0) host controller driver
- USB enumeration and device discovery
- USB HID class driver
- HID report descriptor parser
- Keyboard report handling

**Complexity:** Significant undertaking. XHCI alone is complex (command rings, event rings, transfer rings, device contexts). Consider as a future milestone after core OS functionality is stable.

**Alternative:** For near-term real hardware testing, enable "USB Legacy Support" in BIOS to get PS/2 emulation for USB keyboards.
