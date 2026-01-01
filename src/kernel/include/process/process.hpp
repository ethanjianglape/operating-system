#pragma once

#include "containers/kvector.hpp"
#include <cstddef>
#include <cstdint>

namespace process {
    struct ProcessAllocation {
        std::uintptr_t virt_addr;
        std::size_t num_pages;
    };
    
    struct Process {
        std::size_t pid;
        std::uint8_t* buffer;
        std::uintptr_t entry;
        kvector<ProcessAllocation> allocations;
    };


    
    void load(std::uint8_t* buffer, std::size_t size);
}
