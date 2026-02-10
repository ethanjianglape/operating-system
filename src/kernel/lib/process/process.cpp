#include "containers/kvector.hpp"
#include "fmt/fmt.hpp"
#include <cstddef>
#include <cstdint>
#include <process/process.hpp>
#include <process/elf.hpp>
#include <log/log.hpp>
#include <arch.hpp>
#include <crt/crt.h>
#include <memory/pmm.hpp>
#include <memory/slab.hpp>

namespace process {
    constexpr std::uintptr_t USER_STACK_BASE = 0x00800000;
    constexpr std::size_t    USER_STACK_SIZE = 16 * 1024; // 16KiB
    constexpr std::uintptr_t USER_STACK_TOP  = USER_STACK_BASE + USER_STACK_SIZE;

    constexpr std::uintptr_t KERNEL_STACK_SIZE = 16 * 1024;

    static std::uint64_t g_pid = 1;

    extern "C" void userspace_entry_trampoline();

    Process* load_elf(std::uint8_t* buffer, std::size_t size) {
        elf::Elf64_File file = elf::parse_file(buffer, size);

        if (!file.is_valid_elf) {
            return nullptr;
        }

        arch::vmm::PageTableEntry* pml4 = arch::vmm::create_user_pml4();
        arch::vmm::switch_pml4(pml4);

        auto* p = new Process{};

        p->pid = g_pid++;
        p->state = ProcessState::READY;
        p->wait_reason = WaitReason::NONE;
        p->exit_status = 0;
        p->entry = file.entry;
        p->heap_break = 0;
        p->pml4 = pml4;
        p->fd_table = {};
        p->kernel_stack = new std::uint8_t[KERNEL_STACK_SIZE];
        p->kernel_rsp = reinterpret_cast<std::uintptr_t>(p->kernel_stack + KERNEL_STACK_SIZE);
        p->wake_time_ms = 0;
        p->has_kernel_context = true;
        p->has_user_context = true;
        p->working_dir = "/";
        p->mmap_min_addr = 65536; // based on /proc/sys/vm/mmap_min_addr in Linux

        for (const elf::Elf64_ProgramHeader& header : file.program_headers) {
            auto virt = header.p_vaddr;
            auto file_size = header.p_filesz;
            auto mem_size = header.p_memsz;
            auto offset = header.p_offset;

            std::size_t code_pages = arch::vmm::map_mem_at(pml4, virt, mem_size, arch::vmm::PAGE_USER | arch::vmm::PAGE_WRITE);

            p->allocations.push_back(ProcessAllocation{
                .virt_addr = virt,
                .num_pages = code_pages
            });

            memcpy(reinterpret_cast<void*>(virt),
                   reinterpret_cast<void*>(buffer + offset),
                   file_size);

            if (mem_size > file_size) {
                memset(reinterpret_cast<void*>(virt + file_size), 0, mem_size - file_size);
            }

            std::uintptr_t segment_end = virt + mem_size;

            if (segment_end > p->heap_break) {
                p->heap_break = (segment_end + 0xFFF) & ~0xFFF;
            }
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

        p->context_frame = reinterpret_cast<arch::context::ContextFrame*>(p->kernel_rsp - sizeof(arch::context::ContextFrame));
        p->context_frame->r15 = p->rip;
        p->context_frame->r14 = p->rsp;
        p->context_frame->r13 = 0xDEADBEEF; // Magic numbers to help with debugging
        p->context_frame->r12 = 0xABABABAB;
        p->context_frame->rbp = 0x77777777;
        p->context_frame->rbx = 0x12345678;
        p->context_frame->rip = reinterpret_cast<std::uintptr_t>(userspace_entry_trampoline);
        p->kernel_rsp_saved = reinterpret_cast<std::uintptr_t>(p->context_frame);

        arch::vmm::switch_kernel_pml4();

        log::debug("Created process ", p->pid);
        log::debug("  kernel_stack @ ", fmt::hex{p->kernel_stack});
        log::debug("  kernel_rsp = ", fmt::hex{p->kernel_rsp});
        log::debug("  context_frame @ ", fmt::hex{p->context_frame});
        log::debug("  context_frame->r15 = ", fmt::hex{p->context_frame->r15});

        log::debug("p2 context_frame virt = ", fmt::hex{(uint64_t)p->context_frame});
        log::debug("p2 context_frame phys = ", fmt::hex{arch::vmm::hhdm_virt_to_phys(p->context_frame)});

        return p;
    }
    
    Process* create_process(std::uint8_t* buffer, std::size_t size) {
        if (buffer == nullptr) {
            log::error("Attempt to load program at NULL");
            return nullptr;
        }
        
        Process* elf = load_elf(buffer, size);

        return elf;
    }

    void terminate_process(Process* proc) {
        log::info("========================================");
        log::info("Terminating process ", proc->pid);
        log::info("========================================");

        auto frames_before = pmm::get_free_frames();
        auto slabs_before = slab::total_slabs();

        for (auto& fd : proc->fd_table) {
            fd.inode->ops->close(&fd);
        }

        for (auto& allocation : proc->allocations) {
            arch::vmm::unmap_mem_at(proc->pml4, allocation.virt_addr, allocation.num_pages);
        }

        arch::vmm::free_page_tables(proc->pml4);
        delete[] proc->kernel_stack;
        delete proc;

        auto frames_after = pmm::get_free_frames();
        auto slabs_after = slab::total_slabs();

        log::info("PMM frames: ", frames_before, " -> ", frames_after,
                  " (+", frames_after - frames_before, ")");
        log::info("Slabs: ", slabs_before, " -> ", slabs_after);
        log::info("========================================");
    }
}
