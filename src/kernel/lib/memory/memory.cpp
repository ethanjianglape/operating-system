#include "arch/i686/paging/paging.hpp"
#include <kernel/memory/memory.hpp>

#include <cstddef>

namespace kernel {
    void* kmalloc(std::size_t size) {
        std::size_t pages_needed = (size / 4096) + 1;

        return i686::paging::alloc_kernel_page(pages_needed);
    }

    void kfree(void* ptr) {

    }
}
