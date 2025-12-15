#include "kernel/log/log.hpp"
#include "kernel/panic/panic.hpp"
#include <kernel/memory/memory.hpp>

#include <cstdint>
#include <cstddef>

namespace kernel::pmm {
    constexpr std::uint32_t PAGE_BITMAP_ENTRY_SIZE = sizeof(std::uint32_t) * 8;
    constexpr std::uint32_t PAGE_BITMAP_SIZE = NUM_PAGES / PAGE_BITMAP_ENTRY_SIZE;
    constexpr std::uint32_t PAGE_FREE = 0;
    constexpr std::uint32_t PAGE_USED = 1;
    
    // 1 bit used per page, 0 = free, 1 = used
    static std::uint32_t page_bitmap[PAGE_BITMAP_SIZE];

    bool is_page_free(std::size_t page) {
        const std::size_t index = page / PAGE_BITMAP_ENTRY_SIZE;
        const std::size_t offset = page % PAGE_BITMAP_ENTRY_SIZE;
        const std::uint32_t entry = page_bitmap[index];
        const std::uint32_t value = entry & (PAGE_USED << offset);

        return value == PAGE_FREE;
    }

    void set_page_used(std::size_t page) {
        const std::size_t index = page / PAGE_BITMAP_ENTRY_SIZE;
        const std::size_t offset = page % PAGE_BITMAP_ENTRY_SIZE;

        page_bitmap[index] |= (PAGE_USED << offset);
    }

    void set_page_free(std::size_t page) {
        const std::size_t index = page / PAGE_BITMAP_ENTRY_SIZE;
        const std::size_t offset = page % PAGE_BITMAP_ENTRY_SIZE;
        const std::size_t mask = ~(PAGE_USED << offset);

        page_bitmap[index] &= mask;
    }

    void init() {
        log::init_start("Physical Memory Management");
        
        // all pages set to used by default
        for (std::size_t i = 0; i < PAGE_BITMAP_SIZE; i++) {
            page_bitmap[i] = 0xFFFFFFFF;
        }

        std::size_t first_page = 256; // skip first 1MiB of pages
        std::size_t last_page = NUM_PAGES - 1;

        for (std::size_t page = first_page; page <= last_page; page++) {
            set_page_free(page);
        }

        // kernel exists from 2-4MiB in physical memory
        std::size_t kernel_first_page = 512;
        std::size_t kernel_last_page = 1024;

        for (std::size_t page = kernel_first_page; page <= kernel_last_page; page++) {
            set_page_used(page);
        }

        log::init_end("Physical Memory Management");
    }

    void* alloc_page() {
        std::size_t page = 0;

        while (page < NUM_PAGES) {
            if (is_page_free(page)) {
                set_page_used(page);
                break;
            }

            page++;
        }

        if (page >= NUM_PAGES) {
            kernel::panic("Cannot allocate any more physical memory pages");
        }
        
        return (void*)(page * PAGE_SIZE);
    }
}
