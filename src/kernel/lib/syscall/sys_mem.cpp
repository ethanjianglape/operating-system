#include "arch/x86_64/memory/vmm.hpp"
#include "kpanic/kpanic.hpp"
#include "log/log.hpp"
#include <syscall/sys_mem.hpp>
#include <process/process.hpp>
#include <arch.hpp>

namespace syscall {
    std::uintptr_t sys_brk(void* addr) {
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
        std::size_t pages = arch::vmm::map_mem_at(proc->pml4, hb_start, size, arch::vmm::PAGE_USER | arch::vmm::PAGE_WRITE);

        proc->heap_break = hb_end;
        proc->allocations.push_back(process::ProcessAllocation{
            .virt_addr = hb_start,
            .num_pages = pages,
        });
        
        return hb_end;
    }

    std::uintptr_t sys_mmap(void* addr_ptr, std::size_t length, int prot, int flags, int fd, std::size_t offset) {
        (void)prot;
        (void)fd;
        (void)offset;
        
        if ((flags & MAP_ANONYMOUS) == 0) {
            log::warn("Invalid call to sys_mmap with flags = ", flags, ", only MAP_ANONYMOUS supported for now.");
            return static_cast<std::uintptr_t>(-1);
        }

        auto* proc = arch::percpu::current_process();
        auto addr = reinterpret_cast<std::uintptr_t>(addr_ptr);

        if (addr_ptr == nullptr || addr < proc->mmap_min_addr) {
            addr = proc->mmap_min_addr;
        }

        int vmm_flags = arch::vmm::PAGE_WRITE | arch::vmm::PAGE_USER;
        arch::vmm::MemoryAllocation allocation = arch::vmm::try_map_mem_at(proc->pml4, addr, length, vmm_flags);

        proc->allocations.push_back(process::ProcessAllocation{
            .virt_addr = allocation.virt_addr,
            .num_pages = allocation.num_pages
        });

        return allocation.virt_addr;
    }

    int sys_munmap(void* addr, std::size_t length) {
        (void)addr;
        (void)length;
        
        return 0;
    }
}
