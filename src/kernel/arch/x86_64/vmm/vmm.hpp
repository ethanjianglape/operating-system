#pragma once

#include <concepts>
#include <cstdint>

namespace x86_64::vmm {
    extern "C" char KERNEL_VIRT_BASE[];
    
    constexpr std::uint32_t NUM_PT_ENTRIES  = 512;
    constexpr std::uint32_t PAGE_SIZE       = 4096;

    constexpr std::uint32_t PAGE_PRESENT       = 0x01; // p
    constexpr std::uint32_t PAGE_WRITE         = 0x02; // rw
    constexpr std::uint32_t PAGE_USER          = 0x04; // us
    constexpr std::uint32_t PAGE_CACHE_DISABLE = 0x06; // pcd

    struct page_directory_entry {
        std::uint32_t p    : 1;  // bit 0: present
        std::uint32_t rw   : 1;  // bit 1: read/write
        std::uint32_t us   : 1;  // bit 2: user/supervisor access
        std::uint32_t pwt  : 1;  // bit 3: write-through
        std::uint32_t pcd  : 1;  // bit 4: cached disabled
        std::uint32_t a    : 1;  // bit 5: accessed
        std::uint32_t ign  : 1;  // bit 6: ignored for 4KB pages
        std::uint32_t ps   : 1;  // bit 7: page size (always 0 for 4KB page)
        std::uint32_t avl  : 4;  // bit 8-11: available (bit 8 ignored by hardware, only bits 9-11 in use)
        std::uint32_t addr : 20; // bit 12-31: page table address >> 12
    };

    static_assert(sizeof(page_directory_entry) == 4, "PDE must be 32 bits");

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
    
    void init(std::uintptr_t hhdm_offset);

    std::uintptr_t map_hddm_page(std::uintptr_t phys, std::uint32_t flags);
    void map_page(std::uintptr_t virt, std::uintptr_t phys, std::uint32_t flags);

    void* alloc_contiguous_memory(std::size_t bytes);
    void* alloc_contiguous_pages(std::size_t num_pages);

    template <typename T>
    inline std::uintptr_t virt_to_phys(T addr) {
        return reinterpret_cast<std::uintptr_t>(addr) - get_hhdm_offset();
    }

    template <typename T>
    inline T phys_to_virt(std::unsigned_integral auto phys) {
        return reinterpret_cast<T>(phys + get_hhdm_offset());
    }
}
