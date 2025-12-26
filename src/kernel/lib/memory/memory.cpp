#include <memory/memory.hpp>
#include <log/log.hpp>
#include <arch.hpp>

#include <cstddef>

void* kmalloc(std::size_t size) {
    if (size == 0) {
        log::warn("kmalloc(0) returns NULL");

        return nullptr;
    }

    return arch::vmm::alloc_contiguous_memory(size);
}

void kfree(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    arch::vmm::free_contiguous_memory(ptr);
}
