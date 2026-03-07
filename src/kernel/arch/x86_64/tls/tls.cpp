#include "tls.hpp"

#include <arch/x86_64/cpu/cpu.hpp>

namespace x86_64::tls {
    void set_fs_base(std::uintptr_t addr) {
        cpu::wrmsr(MSR_FS_BASE, addr);        
    }
}
