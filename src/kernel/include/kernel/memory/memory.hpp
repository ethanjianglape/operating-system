#pragma once

#include <cstddef>

namespace kernel {
    void* kmalloc(std::size_t size);
    
    void kfree(void* ptr);

    namespace pmm {
        // x86 uses 4096 byte pages
        inline constexpr std::size_t PAGE_SIZE = 4096;

        // For now, until we are ready to implement GRUB multiboot2 memory map
        // parsing, just assume 128MiB of physical memory available
        // 32768 * 4096 = 128MiB
        inline constexpr std::size_t NUM_PAGES = 32768;
        
        void init();

        void* alloc_page();
    }
}
