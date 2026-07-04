#include "log/log.hpp"
#include <arch.hpp>
#include <process/process.hpp>
#include <syscall/sys_mem.hpp>

namespace syscall {

std::uintptr_t sys_brk(void* addr)
{
    auto* proc = arch::percpu::current_process();

    if (addr == nullptr) {
        return proc->heap_break;
    }

    std::uintptr_t hb_start = proc->heap_break;
    std::uintptr_t hb_end = reinterpret_cast<std::uintptr_t>(addr);

    if (hb_end <= hb_start) {
        return hb_start;
    }

    std::uintptr_t size = hb_end - hb_start;
    std::size_t pages = arch::vmm::map_pages(proc->pml4, hb_start, size, arch::vmm::PAGE_USER | arch::vmm::PAGE_WRITE);

    proc->heap_break = hb_end;
    proc->allocations.emplace_back(hb_start, pages);

    return hb_end;
}

std::uintptr_t sys_mmap(void*, std::size_t length, int, int flags, int, std::size_t)
{
    if ((flags & linux::MAP_ANONYMOUS) == 0) {
        log::warn("Invalid call to sys_mmap with flags = ", flags, ", only MAP_ANONYMOUS supported for now.");
        return static_cast<std::uintptr_t>(-1);
    }

    auto* proc = arch::percpu::current_process();

    log::debug("sys mmap");

    int vmm_flags = arch::vmm::PAGE_WRITE | arch::vmm::PAGE_USER;
    void* virt_addr = arch::vmm::map_heap_pages(proc->pml4, &proc->uheap, length, vmm_flags);

    log::debugf("sys_mmap virt = {}", virt_addr);

    proc->uheap_allocations.push_back(virt_addr);

    return reinterpret_cast<std::uintptr_t>(virt_addr);
}

int sys_munmap(void*, std::size_t)
{
    return 0;
}

}
