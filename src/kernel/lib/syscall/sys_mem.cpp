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
            .virt_addr = hb_end,
            .num_pages = pages,
        });
        
        return hb_end;
    }
}
