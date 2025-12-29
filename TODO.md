# Future Improvements

## Index

- [x] [Logging and Console Separation](#logging-and-console-separation)
- [x] [Dynamic Collections (kvector/kstring)](#dynamic-collections)
- [x] [Slab Allocator](#slab-allocator)
- [x] [Unit Testing Framework](#unit-testing-framework)
- [~] [VFS and Initramfs](#vfs-and-initramfs)
- [ ] [Userspace Program Loading](#userspace-program-loading)
- [ ] [IOAPIC GSI Mapping](#ioapic-gsi-mapping)
- [ ] [PS/2 Controller Initialization](#ps2-controller-initialization)
- [ ] [USB HID Keyboard Support](#usb-hid-keyboard-support)
- [x] [Namespace Cleanup](#namespace-cleanup)

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
- kprint/log:: writes directly to serial output
- console:: is purely for TTY/user interaction
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

**Status:** Complete

**Prerequisite:** `kfree` implementation (VMM header-based size tracking, PMM `free_frames`)

**Why needed:** Shell argument parsing, path handling, VFS entries, command history all benefit from dynamic containers. Working without them is increasingly painful.

**Components:**
- `kvector<T>` - dynamic array with `push_back`, `pop_back`, `operator[]`, iterators
- `kstring` - dynamic string with concatenation, `c_str()`, comparison

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

## Slab Allocator

**Status:** Complete

**Goal:** Efficient allocation of small objects (<=1024 bytes) without wasting full pages.

**Implemented:**
- Size classes: 32, 64, 128, 256, 512, 1024 bytes
- Slab metadata embedded in page header (40 bytes)
- Per-chunk free list with O(1) alloc/free within a slab
- Doubly-linked list of slabs per size class for O(1) insertion and removal
- Newest slabs searched first (most likely to have free chunks)
- Magic number validation for slab identification from arbitrary addresses
- Empty slab reclamation: slabs are freed back to VMM when all chunks are returned (keeps at least one slab per size class)

**Optimizations:**
- `Slab::size_class_index` (1 byte) instead of full size (8 bytes) - enables O(1) SizeClass lookup via direct array indexing in `free()`
- `chunks_per_slab` stored in SizeClass (computed once at compile time) rather than per-slab
- Compact 40-byte header leaves 126 chunks per slab for 32-byte allocations

**Location:** `kernel/lib/memory/slab.cpp`, `kernel/include/memory/slab.hpp`

---

## Unit Testing Framework

**Status:** Complete

**Goal:** In-kernel unit tests to validate critical subsystems, especially the memory allocation pipeline.

**Implementation:**
- Test framework in `test/test.hpp` and `test/test.cpp`
- Assertions using `std::source_location` (no macros)
- `KERNEL_TESTS` compile flag enables tests
- `KERNEL_TESTS_QUIET` suppresses passing test output (default ON)
- Tests run at boot, before shell

**Test suites (160+ tests):**
- `test_pmm` - physical frame allocation, contiguous frames
- `test_vmm` - raw pages, tracked allocations
- `test_slab` - size classes, chunk management, slab lifecycle
- `test_kmalloc` - routing between slab/VMM, boundary conditions
- `test_kvector` - all container operations, copy/move semantics
- `test_kstring` - string manipulation, concatenation, substr

**PMM sanity check:** Compares free frame count before/after all tests to detect memory leaks in test code itself.

**Bugs caught:**
- VMM alloc/free API mismatch (would have caused undefined behavior)
- kstring insert/erase off-by-one errors
- kstring substr loop condition bugs

**Location:** `kernel/test/`

---

## VFS and Initramfs

**Status:** Partially complete

**Prerequisite:** Dynamic collections (kvector/kstring)

**Goal:** Enable shell commands like `ls` and `cat` to interact with files.

**Architecture:**
```
User/Shell: vfs::open(), vfs::read(), vfs::readdir()
                    │
                    ▼
               VFS layer (path canonicalization, mount resolution)
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
    initramfs     FAT32      ISO9660
    (memory)      (disk)       (CD)
```

**Completed:**
- Initramfs loaded as Limine module
- Tar parser with relative path handling (strips `./` prefix)
- Initramfs mounted at `/` as root filesystem
- VFS path canonicalization (resolves `.`, `..`, redundant slashes)
- `vfs::stat()` for file type checking (FILE, DIRECTORY, NOT_FOUND)
- `vfs::readdir()` for directory listing
- Shell commands: `cd`, `ls`, `cat`, `pwd` (in prompt)
- Relative and absolute path support
- Current working directory tracking

**Remaining cleanup:**
- Error handling consistency
- Code organization/cleanup
- Additional VFS operations as needed

**Future backends:**
- FAT32 for USB drives / SD cards
- ext2 for more Unix-like semantics
- ISO9660 for CD-ROM images

---

## Userspace Program Loading

**Status:** Not started

**Prerequisite:** VFS (to read executables), VMM (to map user pages)

**Goal:** Load and execute ELF binaries from initramfs in ring 3.

**Build infrastructure (complete):**
- `src/user/` directory for userspace programs
- Separate CMakeLists.txt with freestanding flags (no `-mcmodel=kernel`)
- Custom linker script (`user.ld`) loads at 0x400000
- Output to `initramfs/bin/` for VFS access
- Minimal `hello.c` test program (infinite loop)

**Kernel work required:**
- ELF header parsing (verify magic, read program headers)
- Load PT_LOAD segments into user address space
- Set up user page tables with USER flag
- Allocate and map user stack
- Context switch to ring 3 (`iretq` with user segments)
- Syscall interface for user programs to interact with kernel

**Initial syscalls needed:**
- `exit` - terminate program
- `write` - output to console (for hello world)

**Future:**
- More syscalls (read, open, etc.)
- Process management (fork, exec, wait)
- Signal handling

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

---

## Namespace Cleanup

**Status:** Complete

**Final structure:**
- Removed `kernel::` namespace entirely (redundant - we're in the kernel)
- Flat subsystem namespaces: `pmm::`, `log::`, `tty::`, `console::`, `fs::`
- Global k-prefixed utilities: `kstring`, `kvector`, `kprint()`, `kprintln()`
- Architecture code: `x86_64::vmm`, `x86_64::drivers::apic`, etc.
- Architecture abstraction: `arch::` namespace aliases in `include/arch.hpp`
- Implementation details: `subsystem_detail::` (not bare `detail::`)

**File structure:**
- `include/` - flat headers (no `include/kernel/` nesting)
- `lib/` - implementations (mirrors include/)
- `arch/x86_64/` - self-contained, headers next to source

**Rules:**
- `lib/` code uses `arch::` abstraction, never `x86_64::` directly
- Only `include/arch.hpp` bridges architectures
- `#include <...>` for non-local, `#include "..."` for same-directory only

See `src/kernel/CONVENTIONS.md` for full details.
