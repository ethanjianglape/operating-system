#include "syscall.hpp"
#include "syscall/fd/syscall_fd.hpp"
#include "syscall/sys_sleep.hpp"

#include <fmt/fmt.hpp>
#include <log/log.hpp>
#include <arch/x86_64/cpu/cpu.hpp>

#include <cstdint>

extern constexpr std::size_t PER_CPU_SELF_OFFSET       = offsetof(x86_64::syscall::PerCPU, self);
extern constexpr std::size_t PER_CPU_KERNEL_RSP_OFFSET = offsetof(x86_64::syscall::PerCPU, kernel_rsp);
extern constexpr std::size_t PER_CPU_USER_RSP_OFFSET   = offsetof(x86_64::syscall::PerCPU, user_rsp);
extern constexpr std::size_t PER_CPU_PROCESS_OFFSET    = offsetof(x86_64::syscall::PerCPU, process);

static int sys_write(int fd, const char* buff, std::size_t count) {
    kstring str{buff, (int)count};

    if (fd == 1) {
        // fd=1 hard coded to print directly to console for now
        console::put(str);
    } else {
        log::info("sys_write:", str);
    }
    
    return count;
}

extern "C"
std::uint64_t int80_dispatcher(x86_64::syscall::SyscallFrame* frame) {
    std::uint32_t syscall_num = frame->rax & 0xFFFFFFFF;
    std::uint32_t arg1        = frame->rbx & 0xFFFFFFFF;
    std::uint32_t arg2        = frame->rcx & 0xFFFFFFFF;
    std::uint32_t arg3        = frame->rdx & 0xFFFFFFFF;

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
    using namespace x86_64::syscall;

    auto* per_cpu = get_per_cpu();
    log::debug("SYSCALL from pid=", per_cpu->process ? per_cpu->process->pid : 0,
               " per_cpu->kernel_rsp=", fmt::hex{per_cpu->kernel_rsp},
               " actual RSP ~", fmt::hex{(uint64_t)frame});

    std::uint64_t syscall_num = frame->rax;
    std::uint64_t arg1 = frame->rdi;
    std::uint64_t arg2 = frame->rsi;
    std::uint64_t arg3 = frame->rdx;
    std::uint64_t arg4 = frame->r10;  // not rcx (clobbered by syscall)
    std::uint64_t arg5 = frame->r8;
    std::uint64_t arg6 = frame->r9;

    log::debug("SYSCALL ", syscall_num);
    log::debug("arg1 = ", arg1, " (", fmt::hex{arg1}, ")");
    log::debug("arg2 = ", arg2, " (", fmt::hex{arg2}, ")");
    log::debug("arg3 = ", arg3, " (", fmt::hex{arg3}, ")");
    log::debug("arg4 = ", arg4, " (", fmt::hex{arg4}, ")");
    log::debug("arg5 = ", arg5, " (", fmt::hex{arg5}, ")");
    log::debug("arg6 = ", arg6, " (", fmt::hex{arg6}, ")");
    log::debug("RCX  = ", fmt::hex{frame->rcx});
    log::debug("R11  = ", fmt::hex{frame->r11});
    log::debug("CS   = ", fmt::hex{frame->cs});
    log::debug("SS   = ", fmt::hex{frame->ss});

    switch (frame->rax) {
    case SYS_WRITE:
        return ::syscall::fd::sys_write(arg1, reinterpret_cast<const char*>(arg2), arg3);
    case SYS_READ:
        return ::syscall::fd::sys_read(arg1, reinterpret_cast<char*>(arg2), arg3);
    case SYS_SLEEP_MS:
        return ::syscall::sys_sleep_ms(arg1);
    default:
        log::error("Unsupported syscall: ", frame->rax);
        return ENOSYS;
    }
}

namespace x86_64::syscall {
    void init_msr() {
        // STAR[63:48] = 0x10 (sysret base: +8=user data, +16=user code)
        // STAR[47:32] = 0x08 (syscall: CS=kernel code, SS=+8=kernel data)
        // STAR[31:0]  = 0x00 (reserved)

        PerCPU* per_cpu_data = new PerCPU{};

        std::uint64_t star   = (0x10UL << 48) | (0x08UL << 32);
        std::uint64_t lstar  = reinterpret_cast<std::uint64_t>(syscall_entry);
        std::uint64_t sfmask = SFMASK_DF | SFMASK_IF | SFMASK_TF;
        std::uint64_t efer   = cpu::rdmsr(MSR_EFER) | EFER_SCE;

        std::uintptr_t gs_base        = reinterpret_cast<std::uintptr_t>(per_cpu_data);
        std::uintptr_t kernel_gs_base = 0;

        per_cpu_data->self = per_cpu_data;
        per_cpu_data->kernel_rsp = 0;
        per_cpu_data->user_rsp = 0;
        per_cpu_data->process = nullptr;

        cpu::wrmsr(MSR_STAR, star);
        cpu::wrmsr(MSR_LSTAR, lstar);
        cpu::wrmsr(MSR_SFMASK, sfmask);
        cpu::wrmsr(MSR_EFER, efer);
        cpu::wrmsr(MSR_GS_BASE, gs_base);
        cpu::wrmsr(MSR_KERNEL_GS_BASE, kernel_gs_base);

        log::info("STAR   = ", fmt::hex{star});
        log::info("LSTAR  = ", fmt::hex{lstar});
        log::info("SFMASK = ", fmt::hex{sfmask});
        log::info("EFER   = ", fmt::hex{efer});
        log::info("Per CPU: kernel stack = ", fmt::hex{per_cpu_data->kernel_rsp}, ", user stack = ", fmt::hex{per_cpu_data->user_rsp});
    }

    PerCPU* get_per_cpu() {
        PerCPU* ptr;
        
        asm volatile("mov %%gs:%c1, %0" : "=r"(ptr) : "i"(PER_CPU_SELF_OFFSET) : "memory");
        
        return ptr;
    }

    void init() {
        log::init_start("Syscall");

        init_msr();
        
        log::init_end("Syscall");
    }
}
