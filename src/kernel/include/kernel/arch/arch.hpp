#pragma once

#include <arch/i686/vmm/vmm.hpp>
#include <arch/i686/cpu/cpu.hpp>

// Allows kernel library code to indirectly access the current CPU architecture
namespace kernel::arch {
    namespace vmm = ::i686::vmm;
    namespace cpu = ::i686::cpu;
}
