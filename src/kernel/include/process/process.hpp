#pragma once

#include "arch/x86_64/context/context.hpp"
#include "containers/kvector.hpp"
#include "fs/fs.hpp"
#include <arch.hpp>

#include <cstddef>
#include <cstdint>

namespace process {
    enum class ProcessState : std::uint8_t {
        RUNNING  = 1,
        READY    = 2,
        BLOCKED  = 3,
        SLEEPING = 4,
        DEAD     = 5,
    };
    
    struct ProcessAllocation {
        std::uintptr_t virt_addr;
        std::size_t num_pages;
    };
    
    struct Process {
        // Process meta info
        std::size_t pid;
        ProcessState state;
        int exit_status;

        // Address space
        arch::vmm::PageTableEntry* pml4;
        std::uintptr_t entry;

        std::uint8_t* kernel_stack;      // Base of kernel stack
        std::uintptr_t kernel_rsp;       // Top of stack (initially)
        std::uintptr_t kernel_rsp_saved; // Used for context switching within syscall
        arch::context::ContextFrame* context_frame;
        bool has_kernel_context;  // Can be resumed via context_switch
        bool has_user_context;    // Can be resumed via schedule/iretq

        // VMM page info
        kvector<ProcessAllocation> allocations;

        // File Descriptors
        kvector<fs::FileDescriptor> fd_table;

        // Sleep
        std::uint64_t wake_time_ms;

        // Save CPU state
        std::uintptr_t rip;
        std::uintptr_t rsp;
        std::uintptr_t rflags;

        std::uint64_t cs, ss;
        std::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        std::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    };
    
    Process* create_process(std::uint8_t* buffer, std::size_t size);

    void terminate_process(Process* process);
}
