#pragma once

#include <arch/x86_64/vmm/vmm.hpp>
#include <arch/x86_64/cpu/cpu.hpp>

// Allows kernel library code to indirectly access the current CPU architecture
namespace kernel::arch {
    namespace vmm = ::x86_64::vmm;
    namespace cpu = ::x86_64::cpu;
}
