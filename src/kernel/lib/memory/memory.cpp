#include "memory/slab.hpp"
#include <memory/memory.hpp>
#include <log/log.hpp>
#include <arch.hpp>

#include <cstddef>

void* kmalloc(std::size_t size) {
    arch::cpu::cli();

    void* ret = nullptr;
    
    if (size == 0) {
        log::warn("kmalloc(0) returns NULL");
    } else if (slab::can_alloc(size)) {
        ret = slab::alloc(size);
    } else {
        ret = arch::vmm::alloc_contiguous_kmem(size);
    }

    arch::cpu::sti();

    return ret;
}

void kfree(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    arch::cpu::cli();

    if (slab::is_slab(ptr)) {
        slab::free(ptr);
    } else {
        arch::vmm::free_contiguous_kmem(ptr);        
    }

    arch::cpu::sti();
}
