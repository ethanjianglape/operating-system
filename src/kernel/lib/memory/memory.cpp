#include "arch/x64/cpu/cpu.hpp"
#include "arch/x64/memory/vmm.hpp"
#include "kassert/kassert.hpp"
#include "memory/slab.hpp"
#include <arch.hpp>
#include <crt/crt.h>
#include <cstdint>
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

    std::uint64_t rflags = arch::cpu::read_rflags();
    arch::cpu::stac();
    memcpy(dst, src, size);
    arch::cpu::write_rflags(rflags);
}

void kcopy_from_user(void* dst, const void* src, std::size_t size)
{
    kassert(arch::vmm::is_user_addr(reinterpret_cast<std::uintptr_t>(src), size));

    std::uint64_t rflags = arch::cpu::read_rflags();
    arch::cpu::stac();
    memcpy(dst, src, size);
    arch::cpu::write_rflags(rflags);
}
