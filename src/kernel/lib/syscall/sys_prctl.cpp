#include <arch.hpp>
#include <process/process.hpp>
#include <syscall/sys_prctl.hpp>

namespace syscall {
std::uint64_t sys_arch_prctl(int code, std::uintptr_t addr)
{
    auto* proc = arch::percpu::current_process();

    switch (code) {
    case ARCH_SET_FS:
        arch::tls::set_fs_base(addr);
        proc->fs_base = addr;
        return 0;
    case ARCH_GET_FS:
        return proc->fs_base;
    }

    return 0;
}
}
