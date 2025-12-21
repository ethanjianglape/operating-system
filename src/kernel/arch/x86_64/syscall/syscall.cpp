#include "syscall.hpp"

#include <kernel/log/log.hpp>
#include <arch/x86_64/cpu/cpu.hpp>

#include <cstdint>

static int sys_write(int fd, const char* buff, std::size_t count) {
    const char* msg = "hello from syscall!";

    kernel::log::info("sys_write: %s", msg);

    return count;
}

extern "C"
std::uint64_t int80_dispatcher(x86_64::syscall::SyscallFrame* frame) {
    std::uint32_t syscall_num = frame->rax & 0xFFFFFFFF;
    std::uint32_t arg1        = frame->rbx & 0xFFFFFFFF;
    std::uint32_t arg2        = frame->rcx & 0xFFFFFFFF;
    std::uint32_t arg3        = frame->rdx & 0xFFFFFFFF;
    std::uint32_t arg4        = frame->rsi & 0xFFFFFFFF;
    std::uint32_t arg5        = frame->rdi & 0xFFFFFFFF;
    std::uint32_t arg6        = frame->rbp & 0xFFFFFFFF;

    switch (syscall_num) {
    case x86_64::syscall::SYSCALL_SYS_WRITE:
        return sys_write(arg1, reinterpret_cast<const char*>(arg2), arg3);
    }

    return 0;
}

extern "C"
void syscall_entry();

extern "C"
std::uint64_t syscall_dispatcher(x86_64::syscall::SyscallFrame* frame) {
    std::uint64_t syscall_num = frame->rax;
    std::uint64_t arg1 = frame->rdi;
    std::uint64_t arg2 = frame->rsi;
    std::uint64_t arg3 = frame->rdx;
    std::uint64_t arg4 = frame->r10;  // not rcx (clobbered by syscall)
    std::uint64_t arg5 = frame->r8;
    std::uint64_t arg6 = frame->r9;

    kernel::log::debug("SYSCALL %d", syscall_num);
    kernel::log::debug("arg1 = %d (%x)", arg1, arg1);
    kernel::log::debug("arg2 = %d (%x)", arg2, arg2);
    kernel::log::debug("arg3 = %d (%x)", arg3, arg3);
    kernel::log::debug("arg4 = %d (%x)", arg4, arg4);
    kernel::log::debug("arg5 = %d (%x)", arg5, arg5);
    kernel::log::debug("arg6 = %d (%x)", arg6, arg6);
    kernel::log::debug("RCX  = %x", frame->rcx);
    kernel::log::debug("R11  = %x", frame->r11);

    return sys_write(arg1, reinterpret_cast<const char*>(arg2), arg3);
}

namespace x86_64::syscall {
    static PerCPU per_cpu_data;

    static std::uint8_t syscall_stack[4096 * 8];
    
    void init_msr() {
        // STAR[63:48] = 0x10 (sysret base: +8=user data, +16=user code)
        // STAR[47:32] = 0x08 (syscall: CS=kernel code, SS=+8=kernel data)
        // STAR[31:0]  = 0x00 (reserved)

        std::uint64_t star   = (0x10UL << 48) | (0x08UL << 32);
        std::uint64_t lstar  = reinterpret_cast<std::uint64_t>(syscall_entry);
        std::uint64_t sfmask = SFMASK_DF | SFMASK_IF | SFMASK_TF;
        std::uint64_t efer = cpu::rdmsr(MSR_EFER) | EFER_SCE;

        per_cpu_data.kernel_rsp = reinterpret_cast<std::uint64_t>(syscall_stack) + sizeof(syscall_stack);
        per_cpu_data.user_rsp = 0;

        cpu::wrmsr(MSR_STAR, star);
        cpu::wrmsr(MSR_LSTAR, lstar);
        cpu::wrmsr(MSR_SFMASK, sfmask);
        cpu::wrmsr(MSR_EFER, efer);
        cpu::wrmsr(MSR_GS_BASE, 0);
        cpu::wrmsr(MSR_KERNEL_GS_BASE, reinterpret_cast<std::uint64_t>(&per_cpu_data));

        kernel::log::info("STAR   = %x", star);
        kernel::log::info("LSTAR  = %x", lstar);
        kernel::log::info("SFMASK = %x", sfmask);
        kernel::log::info("EFER   = %x", efer);
        kernel::log::info("Per CPU: kernel stack = %x, user stack = %x", per_cpu_data.kernel_rsp, per_cpu_data.user_rsp);
    }

    void init() {
        kernel::log::init_start("Syscall");

        init_msr();
        
        kernel::log::init_end("Syscall");
    }
}






