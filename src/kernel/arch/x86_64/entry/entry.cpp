#include "entry.hpp"
#include "syscall/sys_fd.hpp"
#include "syscall/sys_proc.hpp"
#include "syscall/sys_sleep.hpp"

#include <arch/x86_64/percpu/percpu.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <process/process.hpp>
#include <fmt/fmt.hpp>
#include <log/log.hpp>

#include <cstdint>
#include <cerrno>

extern "C"
void syscall_entry();

extern "C"
std::uint64_t syscall_dispatcher(x86_64::entry::SyscallFrame* frame) {
    using namespace x86_64::entry;

    auto* process = x86_64::percpu::current_process();

    process->has_kernel_context = true;
    process->has_user_context = false;

    std::uint64_t arg1 = frame->rdi;
    std::uint64_t arg2 = frame->rsi;
    std::uint64_t arg3 = frame->rdx;

    switch (frame->rax) {
    case SYS_WRITE:
        return syscall::sys_write(arg1, reinterpret_cast<const char*>(arg2), arg3);
    case SYS_READ:
        return syscall::sys_read(arg1, reinterpret_cast<char*>(arg2), arg3);
    case SYS_SLEEP_MS:
        return syscall::sys_sleep_ms(arg1);
    case SYS_LSEEK:
        return syscall::sys_lseek(arg1, arg2, arg3);
    case SYS_GETPID:
        return syscall::sys_getpid();
    default:
        log::error("Unsupported syscall: ", frame->rax);
        return -ENOSYS;
    }
}

namespace x86_64::entry {
    void init() {
        log::init_start("Syscall");

        // STAR[63:48] = 0x10 (sysret base: +8=user data, +16=user code)
        // STAR[47:32] = 0x08 (syscall: CS=kernel code, SS=+8=kernel data)
        // STAR[31:0]  = 0x00 (reserved)

        std::uint64_t star   = (0x10UL << 48) | (0x08UL << 32);
        std::uint64_t lstar  = reinterpret_cast<std::uint64_t>(syscall_entry);
        std::uint64_t sfmask = SFMASK_DF | SFMASK_IF | SFMASK_TF;
        std::uint64_t efer   = cpu::rdmsr(MSR_EFER) | EFER_SCE;

        cpu::wrmsr(MSR_STAR, star);
        cpu::wrmsr(MSR_LSTAR, lstar);
        cpu::wrmsr(MSR_SFMASK, sfmask);
        cpu::wrmsr(MSR_EFER, efer);

        log::info("STAR   = ", fmt::hex{star});
        log::info("LSTAR  = ", fmt::hex{lstar});
        log::info("SFMASK = ", fmt::hex{sfmask});
        log::info("EFER   = ", fmt::hex{efer});

        log::init_end("Syscall");
    }
}
