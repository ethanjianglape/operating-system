# Future Improvements

## Index

- [x] [Logging and Console Separation](#logging-and-console-separation)
- [x] [Dynamic Collections (kvector/kstring)](#dynamic-collections)
- [x] [Slab Allocator](#slab-allocator)
- [x] [Unit Testing Framework](#unit-testing-framework)
- [x] [VFS and Initramfs](#vfs-and-initramfs)
- [x] [Userspace Program Loading](#userspace-program-loading)
- [x] [Basic Process Scheduling](#basic-process-scheduling)
- [x] [IOAPIC GSI Mapping](#ioapic-gsi-mapping)
- [x] [PS/2 Controller Initialization](#ps2-controller-initialization)
- [ ] [USB HID Keyboard Support](#usb-hid-keyboard-support)
- [x] [Namespace Cleanup](#namespace-cleanup)
- [x] [Arch Namespace Collisions](#arch-namespace-collisions)
- [x] [Process Termination and Cleanup](#process-termination-and-cleanup)
- [x] [Basic Linux Binary Compatibility](#basic-linux-binary-compatibility)
- [ ] [Tmpfs (In-Memory Read/Write Filesystem)](#tmpfs-in-memory-readwrite-filesystem)
- [ ] [FAT32 Filesystem](#fat32-filesystem)
- [ ] [Documentation and References](#documentation-and-references)

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

**Status:** Complete

**Prerequisite:** Dynamic collections (kvector/kstring)

**Goal:** Clean, Unix-like VFS with single dispatch for file operations.

**Architecture:**
```
Syscall Layer (sys_fd.cpp)
  sys_open() → fs::open() → Inode*
  sys_read() → fd->inode->ops->read()   ← single dispatch
                    │
                    ▼
fs::open/stat/readdir (fs.cpp)
  - Mount table lookup
  - Path canonicalization
  - Delegates to FileSystem
                    │
        ┌───────────┴───────────┐
        ▼                       ▼
    initramfs (/)           devfs (/dev)
    - ops = fs_file_ops     - tty1: tty_ops
    - FsFileMeta*           - null: null_ops
```

**Core types (fs.hpp):**
- `Inode` - file identity: type, size, ops*, private_data
- `FileDescriptor` - open handle: Inode* pointer, offset, flags
- `FileOps` - fd operations: read, write, close
- `FileSystem` - path operations: open, stat, readdir
- `MountPoint` - filesystem mounted at path

**Key design decisions:**
- Single dispatch: `fd->inode->ops->read()` (no double indirection)
- `FileDescriptor` holds `Inode*` pointer (not embedded copy)
- `FileSystem::open()` returns heap-allocated Inode, `FileOps::close()` frees it
- Char devices use static inodes (singleton, close is no-op)
- Offset handling is FileOps responsibility (fs_file_ops tracks, tty ignores)

**File structure:**
```
include/fs/
├── fs.hpp              # All types + VFS operations
├── fs_file_ops.hpp     # FsFileMeta + get_fs_file_ops()
├── initramfs/
│   └── initramfs.hpp
└── devfs/
    ├── devfs.hpp
    ├── dev_tty.hpp
    └── dev_null.hpp

lib/fs/
├── fs.cpp              # Mount table + path routing
├── fs_file_ops.cpp     # Shared read/write/close for fs files
├── initramfs/
│   ├── initramfs.cpp
│   └── tar.cpp
└── devfs/
    ├── devfs.cpp
    ├── dev_tty.cpp     # TTY: keyboard input, console output
    └── dev_null.cpp    # Null: discard writes, EOF on read
```

**Devices:**
- `/dev/tty1` - TTY with line editing, history, keyboard input
- `/dev/null` - Discards writes, returns EOF on read

**Future backends:**
- FAT32 for USB drives / SD cards
- ext2 for more Unix-like semantics

---

## Userspace Program Loading

**Status:** Complete

**Prerequisite:** VFS (to read executables), VMM (to map user pages)

**Goal:** Load and execute ELF binaries from initramfs in ring 3.

**Implemented:**
- ELF64 parser with header validation (magic, class, machine type)
- Program header parsing to find PT_LOAD segments
- Per-process page tables (`create_user_pml4()` with kernel mappings copied)
- VMM functions parameterized by PML4 for process-specific mappings
- `map_mem_at(pml4, vaddr, size, flags)` maps non-contiguous physical frames
- User stack allocation and mapping
- Process struct with PID, state, PML4, entry point, allocation tracking
- `kvector<ProcessAllocation>` for cleanup on process exit
- Context switch to ring 3 via `iretq` with user segments (cs=0x23, ss=0x1B)
- `sys_write` syscall working (hello world prints to console)

**Build infrastructure:**
- `src/user/` directory for userspace programs
- Separate CMakeLists.txt with freestanding flags (no `-mcmodel=kernel`)
- Custom linker script (`user.ld`) loads at 0x400000
- Output to `initramfs/bin/` for VFS access
- Standard ELF binaries compatible with Linux toolchain

**Key insight:** Programs compiled on Linux with standard gcc run unmodified on this OS. Same ELF format, same x86-64 ABI, same syscall convention.

**Next steps (see Process Scheduling):**
- `sys_exit` to cleanly terminate and free resources
- Scheduler to manage multiple processes
- More syscalls (read, open, etc.)

---

## Basic Process Scheduling

**Status:** Complete

**Implemented:**
- Preemptive scheduling via APIC timer interrupts
- Cooperative scheduling via `sys_yield` for voluntary context switches
- Process states: RUNNING, READY, BLOCKED, DEAD
- Context save/restore (all registers + rip, rsp, rflags)
- CR3 switching for per-process address spaces
- `sys_exit` for process termination
- `sys_sleep_ms` for timed blocking
- Round-robin ready queue

**Architecture:**
```
Timer interrupt fires (preemptive)
   — or —
Process calls sys_yield (cooperative)
        │
        ▼
Save current process state
        │
        ▼
Scheduler picks next READY process
        │
        ▼
Switch CR3 to next process's PML4
        │
        ▼
Restore next process state
        │
        ▼
iretq → process resumes
```

**Future enhancements (separate task):**
- Priority levels
- Multi-CPU support (per-CPU run queues)
- Wait/waitpid for parent-child synchronization
- Improved timer calibration (HPET, TSC frequency via CPUID)

---

## IOAPIC GSI Mapping

**Status:** Complete

**Implemented:**
- MADT Interrupt Source Overrides are now parsed and stored
- `get_gsi_for_irq()` resolves legacy IRQ → GSI with override check
- `get_ioapic_for_gsi()` finds correct IOAPIC by GSI range (multi-IOAPIC support)
- `get_mapped_ioapic_addr()` returns mapped virtual address for IOAPIC
- Polarity/trigger flags from overrides applied when programming redirection entries
- Generic `ioapic_route_irq(irq, vector)` exported for device drivers
- Clear separation: `IRQ_*` constants for hardware IRQs, `VECTOR_*` for IDT vectors

**Architecture:**
```
Device driver calls ioapic_route_irq(IRQ_KEYBOARD, VECTOR_KEYBOARD)
    │
    ▼
get_gsi_for_irq(1) → checks ISO table → returns GSI (may differ from IRQ)
    │
    ▼
get_ioapic_for_gsi(gsi) → finds which IOAPIC handles this GSI
    │
    ▼
get_override_for_irq(1) → gets polarity/trigger flags if ISO exists
    │
    ▼
Programs correct IOAPIC at correct pin with correct flags
```

---

## PS/2 Controller Initialization

**Status:** Complete

**Implemented:**
- Controller existence check (0xFF from status port = no controller)
- Disable both ports during initialization
- Flush output buffer before setup
- Read/modify controller configuration byte (disable IRQs and translation during setup)
- Controller self-test (command 0xAA, expect 0x55)
- Port 1 test (command 0xAB, expect 0x00)
- Enable port 1 after tests pass
- Keyboard device reset (send 0xFF, wait for ACK + self-test OK)
- Enable IRQ1 in configuration after keyboard is ready
- Graceful failure handling (logs errors, continues boot without keyboard)

**Init sequence:**
```
1. Check controller exists (status != 0xFF)
2. Disable ports (0xAD, 0xA7)
3. Flush output buffer
4. Read config, disable IRQs/translation
5. Controller self-test (0xAA → 0x55)
6. Port 1 test (0xAB → 0x00)
7. Enable port 1 (0xAE)
8. Reset keyboard (0xFF → 0xFA, 0xAA)
9. Enable IRQ1 in config
10. Route IRQ1 via IOAPIC, register handler
```

**Modern hardware note:** Many laptops lack real PS/2 controllers. If init fails, the kernel continues without keyboard input. USB HID support (future task) would provide broader compatibility.

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
- Flat subsystem namespaces: `pmm::`, `log::`, `console::`, `fs::`
- Nested fs namespaces: `fs::initramfs::`, `fs::devfs::`, `fs::devfs::tty::`
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

---

## Arch Namespace Collisions

**Status:** Complete

**Problem:** Architecture-specific namespaces mirrored generic namespace names, causing confusion and requiring explicit qualification:
- `x86_64::syscall` vs `::syscall`

**Changes made:**

| Before | After | Purpose |
|--------|-------|---------|
| `x86_64::syscall` | `x86_64::trap` | User→kernel trap mechanism (LSTAR, SYSRET, SyscallFrame) |
| (in entry) | `x86_64::percpu` | Per-CPU data structures (GS segment, kernel RSP) |
| `syscall::fd` | `syscall::` | Flattened syscall namespace |
| `panic()` | `kpanic()` | Variadic template, matches kmalloc naming |

**Additional cleanup:**
- Removed legacy int 0x80 support (adds complexity without value)
- Removed kernel shell code (now handled in userspace)
- Added `arch::percpu::current_process()` helper for common access pattern

**Final structure:**
- `x86_64::trap` - user→kernel transition mechanism (LSTAR/STAR/SFMASK MSR setup)
- `x86_64::percpu` - per-CPU data, GS segment management
- `x86_64::context` - context switching mechanics
- `syscall::` - flat namespace for syscall implementations (sys_read, sys_write, etc.)
- `arch::` aliases all x86_64 namespaces for portable kernel code

---

## Process Termination and Cleanup

**Status:** Complete

**Goal:** Properly terminate processes and reclaim all resources when they exit or are killed.

**Implemented:**
- `sys_exit()` marks process as DEAD and calls `yield_dead()` (never returns)
- `yield_dead()` context switches away from the dying process permanently
- `terminate_dead_processes()` runs on timer interrupt, cleans up DEAD processes
- `terminate_process()` frees all process resources:
  - Closes all file descriptors via `fd->inode->ops->close()`
  - Unmaps user pages and frees physical frames via `unmap_mem_at()`
  - Frees page table hierarchy (PT, PD, PDPT, PML4) via `free_page_tables()`
  - Frees kernel stack via `delete[]`
  - Frees Process struct via `delete`
- Removed from scheduler queue via `klist::erase()`
- Sanity logging shows PMM frames reclaimed and slab counts

**Architecture:**
```
sys_exit(status)
        │
        ▼
Mark process DEAD, call yield_dead()
        │
        ▼
yield_dead() context switches to ready process
  (saves RSP to dead process, but no one will switch back)
        │
        ▼
Timer fires, terminate_dead_processes() runs
        │
        ▼
terminate_process(proc)
  - Close FDs
  - Unmap user pages (frees physical frames)
  - Free page table hierarchy
  - Free kernel stack
  - Free Process struct
        │
        ▼
Remove from scheduler queue
```

**Key insight:** The exiting process can't free its own kernel stack while running on it. Solution: `yield_dead()` switches away first, then cleanup happens later from a different context (timer interrupt handler).

**Future enhancements:**
- Parent-child relationships for `waitpid()`
- Signal-based process termination (SIGKILL, SIGTERM)
- Exit status retrieval by parent process

---

## Basic Linux Binary Compatibility

**Status:** Complete

**Goal:** Run standard C programs compiled with GCC targeting Linux, statically linked with musl libc. This proves the OS implements a real, standard syscall ABI rather than a custom toy interface.

**Implemented:**
- Syscall entry via SYSCALL/SYSRET with correct Linux x86_64 register convention
- Syscall numbers match Linux: read(0), write(1), open(2), close(3), stat(4), fstat(5), lseek(8), mmap(9), munmap(11), brk(12), ioctl(16), writev(20), nanosleep(35), getpid(39), exit(60), getcwd(79), chdir(80), fchdir(81), arch_prctl(158), set_tid_address(218), exit_group(231)
- ELF64 loader handles static ET_EXEC binaries
- Per-process address spaces with user/kernel separation
- brk and mmap (anonymous) for heap allocation
- arch_prctl (ARCH_SET_FS) for TLS support
- ioctl TIOCGWINSZ for terminal detection (enables musl stdio line buffering)
- writev for musl's printf/puts output path
- fd 0/1/2 (stdin/stdout/stderr) initialized to /dev/tty1

**Linux ABI layer (`include/linux/`):**
- `linux::` namespace separates Linux-specific constants from generic kernel code
- `syscall.hpp` — syscall numbers
- `mman.hpp` — mmap/mprotect constants
- `ioctl.hpp` — ioctl constants, struct winsize, struct iovec

**Shell compiled with musl:** The interactive shell (`src/user/shell.c`) is compiled with `musl-gcc -static` and uses standard libc (stdio.h, string.h, unistd.h, sys/mman.h). It runs unmodified on the kernel.

**Why musl over glibc:** musl's static initialization is minimal (arch_prctl, set_tid_address, then straight to main). glibc does significantly more during init (mprotect, mmap, multiple brk calls, etc.).

**Educational value:** This milestone demonstrates that the OS implements enough of the Linux syscall ABI to run unmodified binaries compiled on a standard Linux system. Same ELF format, same x86-64 ABI, same syscall convention — the program doesn't know (or care) that it's not running on Linux.

---

## Tmpfs (In-Memory Read/Write Filesystem)

**Status:** Not started

**Prerequisite:** VFS infrastructure (already complete)

**Goal:** A read/write in-memory filesystem mounted at `/tmp`, giving userspace programs the ability to create, write, read, and delete files.

**Why needed:** The current filesystem is read-only (initramfs) or device-only (devfs). Userspace programs have no way to create or write files. Tmpfs fills this gap without requiring a disk driver.

**Required syscalls/features:**
- `sys_open` with `O_CREAT`, `O_WRONLY`, `O_RDWR`, `O_TRUNC`, `O_APPEND` flags
- `sys_write` to regular files (growing file data dynamically)
- `sys_mkdir` for directory creation
- `sys_unlink` for file deletion
- `sys_stat` on tmpfs files

**Architecture:**
```
VFS mount table
    │
    ├── /      → initramfs (read-only)
    ├── /dev   → devfs (char devices)
    └── /tmp   → tmpfs (read/write, in-memory)

tmpfs internals:
    TmpfsNode (directory)
      ├── "hello.txt" → TmpfsNode (file, data = kmalloc'd buffer)
      ├── "subdir"    → TmpfsNode (directory)
      │     └── ...
      └── ...
```

**Key design decisions:**
- Tree of `TmpfsNode` structs, each with name, type, children (dir) or data buffer (file)
- File data backed by kmalloc — grows dynamically on write
- Implements `FileSystem` interface (open, stat, readdir) + read/write `FileOps`
- No size limits (bounded only by available kernel memory)

**Verification:** Userspace test program compiled with musl that creates a file in `/tmp`, writes to it, closes it, reopens for reading, reads it back, and verifies contents.

**Educational value:** Teaches the write side of filesystem operations — the same structural concepts (directory entries, file data management, inode allocation) that apply to on-disk filesystems, just backed by memory instead of disk blocks.

---

## FAT32 Filesystem

**Status:** Not started (future milestone)

**Prerequisite:** Tmpfs (to establish read/write VFS patterns), block device driver

**Goal:** Read/write FAT32 filesystem support, enabling persistent storage across reboots.

**Why FAT32:**
- Well-documented, simple on-disk structures
- Universal compatibility (USB drives, SD cards, EFI System Partition)
- Straightforward enough to diagram and explain
- No journaling complexity

**Required components:**
1. **Block device driver** — virtio-blk for QEMU testing, AHCI for real hardware
2. **Partition table parsing** — GPT or MBR to find FAT32 partitions
3. **FAT32 driver** — BPB parsing, FAT chain traversal, directory entry reading/writing, cluster allocation

**On-disk structures:**
```
Boot sector (BPB)
    │
    ├── FAT (File Allocation Table) — linked list of clusters
    │     cluster 2 → 3 → 7 → EOF
    │
    └── Data region — actual file/directory contents
          cluster 2: [first 4096 bytes of file]
          cluster 3: [next 4096 bytes]
          cluster 7: [final bytes]
```

**Bridge to real systems:** Linux's FAT32 implementation (`fs/fat/`) does the same thing with additional concerns: long filename support (VFAT), character encoding, mount options, permissions mapping. Same core idea, more production concerns.

---

## Documentation and References

**Status:** Not started

**Goal:** Create a `docs/` folder with references and explanations for major OS concepts implemented in this project.

**Structure:**
```
docs/
├── REFERENCES.md      # Links to external specs and resources
├── pmm.md             # Physical memory management explained
├── vmm.md             # Virtual memory and paging
├── slab.md            # Slab allocator design
├── vfs.md             # Virtual filesystem layer
├── elf.md             # ELF format (what we actually use)
├── interrupts.md      # IDT, APIC, interrupt handling
└── syscalls.md        # System call interface
```

**Content approach:**
- Focus on "the 10% we actually use" rather than full spec dumps
- Link to authoritative external sources (ELF spec, Intel manuals, OSDev wiki)
- Explain our implementation choices and trade-offs
- Include diagrams where helpful
- Cross-reference with Linux equivalents per project philosophy

**Key references to include:**
- ELF64 specification
- Intel SDM (relevant volumes)
- System V ABI AMD64 supplement
- ACPI specification
- Limine boot protocol

**Benefits:**
- Supports educational mission of the project
- Provides context for readers exploring the code
- Documents design decisions for future reference
