#pragma once

#include <cstdint>

namespace i386::paging {
    inline constexpr std::uint32_t NUM_PDT_ENTRIES = 1024;
    inline constexpr std::uint32_t NUM_PT_ENTRIES  = 1024;
    inline constexpr std::uint32_t PAGE_SIZE       = 4096;
    
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
}
