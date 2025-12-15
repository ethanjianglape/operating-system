#include <kernel/memory/memory.hpp>
#include <kernel/log/log.hpp>
#include <kernel/arch/arch.hpp>

#include <cstddef>

namespace kernel {
    void* kmalloc(std::size_t size) {
        if (size == 0) {
            log::warn("kmalloc(0) returns NULL");
            
            return nullptr;
        }

        return arch::vmm::alloc_kernel_memory(size);
    }

    void kfree(void* ptr) {
        if (ptr == nullptr) {
            log::warn("kfree(NULL) is undefined behavior");
        }
    }
}
