#pragma once

#include "containers/kvector.hpp"
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

        // Save CPU state
        std::uintptr_t rip;
        std::uintptr_t rsp;
        std::uintptr_t rflags;
    };
    
    void load(std::uint8_t* buffer, std::size_t size);
}
