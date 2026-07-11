#pragma once

#include <cstddef>
#include <cstdint>

namespace x64::vmm {

extern "C" char KERNEL_VIRT_BASE[];

// ============================================================================
// Types & constants
// ============================================================================

constexpr std::size_t NUM_PT_ENTRIES = 512;
constexpr std::size_t PAGE_SIZE = 4096;
constexpr std::size_t PAGE_MASK = 0xFFF;

constexpr std::uint32_t PAGE_WRITE = 0x02;
constexpr std::uint32_t PAGE_USER = 0x04;
constexpr std::uint32_t PAGE_CACHE_DISABLE = 0x08;
constexpr std::uint32_t PAGE_GLOBAL = 0x10;
constexpr std::uint32_t PAGE_NX = 0x20;

// PML4 entry — points to a PDPT. Bit 7 is reserved and must be 0.
struct PML4E {
    std::uint64_t p    : 1;  // present
    std::uint64_t rw   : 1;  // read/write
    std::uint64_t us   : 1;  // user/supervisor
    std::uint64_t pwt  : 1;  // write-through
    std::uint64_t pcd  : 1;  // cache disable
    std::uint64_t a    : 1;  // accessed
    std::uint64_t      : 1;  // ignored
    std::uint64_t      : 1;  // reserved (must be 0)
    std::uint64_t      : 1;  // ignored
    std::uint64_t avl  : 3;  // available to software
    std::uint64_t addr : 40; // physical address of PDPT
    std::uint64_t      : 7;  // ignored
    std::uint64_t mpk  : 4;  // memory protection key
    std::uint64_t nx   : 1;  // no-execute
};
static_assert(sizeof(PML4E) == 8);

// PDPT entry — points to a PD (ps=0) or maps a 1GB page (ps=1).
struct PDPTE {
    std::uint64_t p    : 1;  // present
    std::uint64_t rw   : 1;  // read/write
    std::uint64_t us   : 1;  // user/supervisor
    std::uint64_t pwt  : 1;  // write-through
    std::uint64_t pcd  : 1;  // cache disable
    std::uint64_t a    : 1;  // accessed
    std::uint64_t      : 1;  // ignored
    std::uint64_t ps   : 1;  // page size (0 = 4KB via PD, 1 = 1GB page)
    std::uint64_t      : 1;  // ignored
    std::uint64_t avl  : 3;  // available to software
    std::uint64_t addr : 40; // physical address of PD
    std::uint64_t      : 7;  // ignored
    std::uint64_t mpk  : 4;  // memory protection key
    std::uint64_t nx   : 1;  // no-execute
};
static_assert(sizeof(PDPTE) == 8);

// PD entry — points to a PT (ps=0) or maps a 2MB page (ps=1).
struct PDE {
    std::uint64_t p    : 1;  // present
    std::uint64_t rw   : 1;  // read/write
    std::uint64_t us   : 1;  // user/supervisor
    std::uint64_t pwt  : 1;  // write-through
    std::uint64_t pcd  : 1;  // cache disable
    std::uint64_t a    : 1;  // accessed
    std::uint64_t      : 1;  // ignored
    std::uint64_t ps   : 1;  // page size (0 = 4KB via PT, 1 = 2MB page)
    std::uint64_t      : 1;  // ignored
    std::uint64_t avl  : 3;  // available to software
    std::uint64_t addr : 40; // physical address of PT
    std::uint64_t      : 7;  // ignored
    std::uint64_t mpk  : 4;  // memory protection key
    std::uint64_t nx   : 1;  // no-execute
};
static_assert(sizeof(PDE) == 8);

// PT entry — maps a 4KB page.
struct PTE {
    std::uint64_t p    : 1;  // present
    std::uint64_t rw   : 1;  // read/write
    std::uint64_t us   : 1;  // user/supervisor
    std::uint64_t pwt  : 1;  // write-through
    std::uint64_t pcd  : 1;  // cache disable
    std::uint64_t a    : 1;  // accessed
    std::uint64_t d    : 1;  // dirty
    std::uint64_t pat  : 1;  // page attribute table
    std::uint64_t g    : 1;  // global
    std::uint64_t avl  : 3;  // available to software
    std::uint64_t addr : 40; // physical address of 4KB page
    std::uint64_t      : 7;  // ignored
    std::uint64_t mpk  : 4;  // memory protection key
    std::uint64_t nx   : 1;  // no-execute
};
static_assert(sizeof(PTE) == 8);

struct Heap {
    std::size_t pml4_idx;
    std::size_t pdpt_idx;
    std::size_t pd_idx;
    std::size_t pt_idx;
};

// ============================================================================
// Lifecycle
// ============================================================================

void init(std::uintptr_t hhdm_offset);

// ============================================================================
// User memory access — SMAP-safe primitives
// ============================================================================

// Returns true if the entire range [addr, addr+size) lies within user space.
bool is_user_addr(void* addr);
bool is_user_addr(const void* addr);
bool is_user_addr(std::uintptr_t addr, std::size_t size);

// ============================================================================
// Kernel memory — VMM owns the address
// ============================================================================

// Allocate/free kernel memory. Size is tracked internally; free with free_kernel().
void* alloc_kernel(std::size_t bytes);
void free_kernel(void* virt);

// Allocate/free a single raw 4KB page with no size header overhead.
void* alloc_kernel_page();
void free_kernel_page(void* virt);

// Map a specific physical address into kernel virtual space.
// VMM chooses the virtual address; returns it at the same page offset as phys.
std::uintptr_t map_phys(std::uintptr_t phys, int flags = 0);

// ============================================================================
// Address space management — caller owns the virtual address
// ============================================================================

// Create a new user-space page table, copying in the kernel mappings.
PML4E* create_user_pml4();
PML4E* clone_user_pml4(PML4E* pml4);

// Initialize a heap cursor for a user address space.
Heap create_user_heap(PML4E* pml4);
Heap clone_user_heap(Heap* existing, PML4E* pml4);

// Map bytes into the next available slot in a heap, allocating physical frames.
void* map_heap_pages(PML4E* pml4, Heap* heap, std::size_t bytes, int flags);

// Low-level: map bytes at a specific virtual address with explicit flags.
std::size_t map_pages(PML4E* pml4, std::uintptr_t virt, std::size_t bytes, int flags);

// Unmap num_pages pages starting at virt, freeing their physical frames.
void unmap_mem_at(PML4E* pml4, std::uintptr_t virt, std::size_t num_pages);

// Free all page table structures for a user address space.
void free_page_tables(PML4E* pml4);

// Switch the active address space.
void switch_pml4(PML4E* pml4);
void switch_kernel_pml4();

// Returns the kernel's PML4, e.g. for initializing a new user process.
PML4E* get_kernel_pml4();

}
