#pragma once

#include "containers/kvector.hpp"
#include "fs/fs.hpp"
#include <arch.hpp>

#include <cstddef>
#include <cstdint>

namespace process {
    enum class ProcessState : std::uint8_t {
        RUNNING = 1,
        READY   = 2,
        BLOCKED = 3
    };
    
    struct ProcessAllocation {
        std::uintptr_t virt_addr;
        std::size_t num_pages;
    };
    
    struct Process {
        // Process meta info
        std::size_t pid;
        ProcessState state;

        // Address space
        arch::vmm::PageTableEntry* pml4;
        std::uintptr_t entry;

        // VMM page info
        kvector<ProcessAllocation> allocations;

        // File Descriptors
        kvector<fs::FileDescriptor> fd_table;

        // Save CPU state
        std::uintptr_t rip;
        std::uintptr_t rsp;
        std::uintptr_t rflags;

        std::uint64_t cs, ss;
        std::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        std::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    };
    
    void load(std::uint8_t* buffer, std::size_t size);
}
