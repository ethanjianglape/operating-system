#pragma once

#include <concepts>
#include <cstdint>

namespace x86_64::vmm {
    extern "C" char KERNEL_VIRT_BASE[];
    
    constexpr std::size_t NUM_PT_ENTRIES  = 512;
    constexpr std::size_t PAGE_SIZE       = 4096;

    constexpr std::uint32_t PAGE_PRESENT       = 0x01; // p
    constexpr std::uint32_t PAGE_WRITE         = 0x02; // rw
    constexpr std::uint32_t PAGE_USER          = 0x04; // us
    constexpr std::uint32_t PAGE_CACHE_DISABLE = 0x06; // pcd

    struct PageTableEntry {
        std::uint64_t p    : 1;
        std::uint64_t rw   : 1;
        std::uint64_t us   : 1;
        std::uint64_t pwt  : 1;
        std::uint64_t pcd  : 1;
        std::uint64_t a    : 1;
        std::uint64_t d    : 1;
        std::uint64_t pat  : 1;
        std::uint64_t g    : 1;
        std::uint64_t avl  : 3;
        std::uint64_t addr : 40;
        std::uint64_t avl2 : 11;
        std::uint64_t nx   : 1;
    };

    static_assert(sizeof(PageTableEntry) == 8, "PTE must be 32 bits");

    std::uintptr_t get_hhdm_offset();

    template <typename T> std::uintptr_t hhdm_virt_to_phys(T addr) {
        return reinterpret_cast<std::uintptr_t>(addr) - get_hhdm_offset();
    }

    template <typename T> T phys_to_virt(std::unsigned_integral auto phys) {
        return reinterpret_cast<T>(phys + get_hhdm_offset());
    }

    void init(std::uintptr_t hhdm_offset);

    std::uintptr_t map_hddm_page(std::uintptr_t phys, std::uint32_t flags);
    void map_kpage(std::uintptr_t virt, std::uintptr_t phys, std::uint32_t flags);

    // Raw single-page HHDM allocation (no header tracking) - for slab allocator
    void* alloc_kpage();
    void free_kpage(void* virt);

    // Tracked HHDM allocation (stores size header) - for general kernel use
    void* alloc_contiguous_kmem(std::size_t bytes);
    void free_contiguous_kmem(void* virt);

    std::size_t map_mem_at(PageTableEntry* pml4, std::uintptr_t virt, std::size_t bytes, std::uint32_t flags);
    void unmap_mem_at(std::uintptr_t virt, std::size_t num_pages);

    PageTableEntry* create_user_pml4();
    
    void switch_pml4(PageTableEntry* pml4);
    void switch_kernel_pml4();
}
