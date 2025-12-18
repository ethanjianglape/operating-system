#pragma once

#include <cstdint>

namespace i686::vmm {
    inline constexpr std::uint32_t NUM_PDT_ENTRIES = 1024;
    inline constexpr std::uint32_t NUM_PT_ENTRIES  = 1024;
    inline constexpr std::uint32_t PAGE_SIZE       = 4096;

    inline constexpr std::uint32_t PAGE_PRESENT       = 0x01; // p
    inline constexpr std::uint32_t PAGE_WRITE         = 0x02; // rw
    inline constexpr std::uint32_t PAGE_USER          = 0x04; // us
    inline constexpr std::uint32_t PAGE_CACHE_DISABLE = 0x06; // pcd

    // Map kernel to higher-half (0xC0000000)
    inline constexpr std::uint32_t KERNEL_VIRT_BASE      = 0xC0000000;                          // 0xC0000000 = 3GiB
    inline constexpr std::uint32_t KERNEL_HIGH_PDE_START = KERNEL_VIRT_BASE >> 22;              // 0xC0000000 >> 22 = 768
    inline constexpr std::uint32_t KERNEL_HIGH_PT_SIZE   = 64;                                  // 64 page tables = 64 * 1024 * 4096 = 256MiB
    inline constexpr std::uint32_t KERNEL_HIGH_PDE_END   = KERNEL_HIGH_PDE_START + KERNEL_HIGH_PT_SIZE;
    inline constexpr std::uint32_t KERNEL_LOW_PDE_START  = 0;                                   // 0x00000000 = start of memory
    inline constexpr std::uint32_t KERNEL_LOW_PT_SIZE    = 8;                                   // 8 page tables = 32MB
    inline constexpr std::uint32_t KERNEL_LOW_PDE_END    = KERNEL_LOW_PDE_START + KERNEL_LOW_PT_SIZE;
    inline constexpr std::uint32_t KERNEL_CODE_SIZE      = 0x00400000;                          // 2MiB for kernel code
    inline constexpr std::uint32_t KERNEL_HEAP_VIRT_BASE = KERNEL_VIRT_BASE + KERNEL_CODE_SIZE; // kernel heap starts after kernel code
    inline constexpr std::uint32_t KERNEL_HEAP_PDE_START = KERNEL_HEAP_VIRT_BASE >> 22;         // 768, same as KERNEL_HIGH_PDE_START
    inline constexpr std::uint32_t KERNEL_HEAP_SIZE      = 0x01000000;                          // 16MiB for kernel heap
    inline constexpr std::uint32_t KERNEL_HEAP_VIRT_END  = KERNEL_HEAP_VIRT_BASE + KERNEL_HEAP_SIZE;

    inline constexpr std::uint32_t USER_PDE_START = KERNEL_LOW_PDE_START + KERNEL_LOW_PT_SIZE; // userspace starts after kernel low
    inline constexpr std::uint32_t USER_PT_SIZE   = 8;                 // give userspace 32MB for now

    inline constexpr std::uint32_t APIC_PDE_START = 1019;

    template <typename T>
    inline constexpr std::uintptr_t virt_to_phys(T addr) {
        return reinterpret_cast<std::uintptr_t>(addr) - KERNEL_VIRT_BASE;
    }

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

    struct page_table_entry {
        std::uint32_t p    : 1;
        std::uint32_t rw   : 1;
        std::uint32_t us   : 1;
        std::uint32_t pwt  : 1;
        std::uint32_t pcd  : 1;
        std::uint32_t a    : 1;
        std::uint32_t d    : 1;
        std::uint32_t pat  : 1;
        std::uint32_t g    : 1;
        std::uint32_t avl  : 3;
        std::uint32_t addr : 20;
    };

    static_assert(sizeof(page_table_entry) == 4, "PTE must be 32 bits");
    
    void init();

    void* map_physical_region(void* physical_addr, std::size_t byte);
    
    void map_page(void* virtual_addr, void* physical_addr, std::uint32_t flags);

    void* alloc_kernel_memory(std::size_t bytes);

    void* alloc_kernel_pages(std::size_t num_pages);
}
