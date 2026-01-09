#include "arch/x86_64/process/process.hpp"
#include "arch/x86_64/memory/vmm.hpp"
#include "containers/kvector.hpp"
#include "scheduler/scheduler.hpp"
#include <cstddef>
#include <cstdint>
#include <process/process.hpp>
#include <process/elf.hpp>
#include <log/log.hpp>
#include <arch.hpp>
#include <crt/crt.h>

namespace process {
    constexpr std::uintptr_t USER_STACK_BASE = 0x00800000;
    constexpr std::size_t    USER_STACK_SIZE = 16 * 1024; // 16KiB
    constexpr std::uintptr_t USER_STACK_TOP  = USER_STACK_BASE + USER_STACK_SIZE;

    static std::uint64_t g_pid = 1;

    void load_elf(std::uint8_t* buffer, std::size_t size) {
        elf::Elf64_File file = elf::parse_file(buffer, size);

        if (!file.is_valid_elf) {
            return;
        }

        arch::vmm::PageTableEntry* pml4 = arch::vmm::create_user_pml4();
        arch::vmm::switch_pml4(pml4);

        auto* p = new Process{};

        p->state = ProcessState::READY;
        p->pid = g_pid++;
        p->entry = file.entry;
        p->pml4 = pml4;
        p->fd_table = {};

        for (const elf::Elf64_ProgramHeader& header : file.program_headers) {
            auto virt = header.p_vaddr;
            auto size = header.p_filesz;
            auto offset = header.p_offset;

            std::size_t code_pages = arch::vmm::map_mem_at(pml4, virt, size, arch::vmm::PAGE_USER);

            p->allocations.push_back(ProcessAllocation{
                .virt_addr = virt,
                .num_pages = code_pages
            });

            memcpy(reinterpret_cast<void*>(virt),
                   reinterpret_cast<void*>(buffer + offset),
                   size);
        }

        std::size_t stack_pages = arch::vmm::map_mem_at(pml4, USER_STACK_BASE, USER_STACK_SIZE, arch::vmm::PAGE_USER);

        p->allocations.push_back(ProcessAllocation{
            .virt_addr = USER_STACK_BASE,
            .num_pages = stack_pages
        });

        // Initial instruction pointer
        p->rip = file.entry;

        // User stack pointer
        p->rsp = USER_STACK_TOP;

        // IF enabled, reserved bit 1 set
        p->rflags = 0x202;

        // Segment selectors with RPL=3 for userspace
        p->cs = 0x20 | 3;  // User Code
        p->ss = 0x18 | 3;  // User Data

        // Zero all general purpose registers
        p->rax = 0;
        p->rbx = 0;
        p->rcx = 0;
        p->rdx = 0;
        p->rsi = 0;
        p->rdi = 0;
        p->rbp = 0;
        p->r8  = 0;
        p->r9  = 0;
        p->r10 = 0;
        p->r11 = 0;
        p->r12 = 0;
        p->r13 = 0;
        p->r14 = 0;
        p->r15 = 0;

        arch::vmm::switch_kernel_pml4();

        scheduler::add_process(p);
    }
    
    void load(std::uint8_t* buffer, std::size_t size) {
        if (buffer == nullptr) {
            log::error("Attempt to load program at NULL");
            return;
        }
        
        load_elf(buffer, size);
    }
}
