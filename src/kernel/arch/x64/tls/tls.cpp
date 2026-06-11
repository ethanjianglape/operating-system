#include "tls.hpp"

#include <arch/x64/cpu/cpu.hpp>

namespace x64::tls {
void set_fs_base(std::uintptr_t addr)
{
    cpu::wrmsr(MSR_FS_BASE, addr);
}
}
