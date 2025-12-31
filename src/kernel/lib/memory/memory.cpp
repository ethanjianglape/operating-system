#include "memory/slab.hpp"
#include <memory/memory.hpp>
#include <log/log.hpp>
#include <arch.hpp>

#include <cstddef>

void* kmalloc(std::size_t size) {
    if (size == 0) {
        log::warn("kmalloc(0) returns NULL");
        return nullptr;
    }

    if (slab::can_alloc(size)) {
        return slab::alloc(size);
    }

    return arch::vmm::alloc_contiguous_kmem(size);
}

void kfree(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    if (slab::is_slab(ptr)) {
        slab::free(ptr);
    } else {
        arch::vmm::free_contiguous_kmem(ptr);        
    }
}
