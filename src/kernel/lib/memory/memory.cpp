#include "arch/x64/memory/vmm.hpp"
#include "kassert/kassert.hpp"
#include "memory/slab.hpp"
#include <arch.hpp>
#include <crt/crt.h>
#include <log/log.hpp>
#include <memory/memory.hpp>

#include <cstddef>

void* kmalloc(std::size_t size)
{
    void* ret = nullptr;

    if (size == 0) {
        log::warn("kmalloc(0) returns NULL");
    } else if (slab::can_alloc(size)) {
        ret = slab::alloc(size);
    } else {
        ret = arch::vmm::alloc_kernel(size);
    }

    return ret;
}

void kfree(void* ptr)
{
    if (ptr == nullptr) {
        return;
    }

    if (slab::is_slab(ptr)) {
        slab::free(ptr);
    } else {
        arch::vmm::free_kernel(ptr);
    }
}

void kcopy_to_user(void* dst, const void* src, std::size_t size)
{
    kassert(arch::vmm::is_user_addr(reinterpret_cast<std::uintptr_t>(dst), size));

    arch::cpu::stac();
    memcpy(dst, src, size);
    arch::cpu::clac();
}

void kcopy_from_user(void* dst, const void* src, std::size_t size)
{
    kassert(arch::vmm::is_user_addr(reinterpret_cast<std::uintptr_t>(src), size));

    arch::cpu::stac();
    memcpy(dst, src, size);
    arch::cpu::clac();
}
