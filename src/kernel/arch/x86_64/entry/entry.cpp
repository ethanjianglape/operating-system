/**
 * @file entry.cpp
 * @brief SYSCALL/SYSRET configuration and syscall dispatch.
 *
 * This file configures the x86-64 SYSCALL instruction and provides the C++
 * dispatcher that routes system calls to their implementations. The actual
 * entry point is in syscall_entry.s (assembly), which handles the low-level
 * register save/restore and stack switching.
 *
 * SYSCALL vs INT 0x80:
 *
 *   SYSCALL is the modern, fast system call mechanism for x86-64. Unlike the
 *   legacy INT 0x80 (software interrupt), SYSCALL doesn't push an interrupt
 *   frame or switch stacks automatically — it just jumps to the handler with
 *   minimal overhead. This makes it ~10x faster but requires careful setup.
 *
 * MSR Configuration:
 *
 *   Four MSRs control SYSCALL behavior:
 *
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │ MSR_EFER (0xC0000080) — Extended Feature Enable Register               │
 *   │   Bit 0 (SCE): Enable SYSCALL/SYSRET instructions                      │
 *   ├─────────────────────────────────────────────────────────────────────────┤
 *   │ MSR_STAR (0xC0000081) — Segment Selectors                              │
 *   │   [63:48] SYSRET CS/SS base (add 16 for CS, add 8 for SS)              │
 *   │   [47:32] SYSCALL CS/SS (CS = value, SS = value + 8)                   │
 *   │   [31:0]  Reserved                                                     │
 *   │                                                                        │
 *   │   Our GDT layout:                                                      │
 *   │     0x08 = kernel code    0x10 = kernel data                           │
 *   │     0x18 = user data      0x20 = user code                             │
 *   │                                                                        │
 *   │   STAR = (0x10 << 48) | (0x08 << 32)                                   │
 *   │     SYSCALL: CS=0x08, SS=0x10 (kernel)                                 │
 *   │     SYSRET:  CS=0x10+16=0x20, SS=0x10+8=0x18 (user, with RPL=3)        │
 *   ├─────────────────────────────────────────────────────────────────────────┤
 *   │ MSR_LSTAR (0xC0000082) — Long Mode SYSCALL Target                      │
 *   │   Address of syscall_entry (in syscall_entry.s)                        │
 *   ├─────────────────────────────────────────────────────────────────────────┤
 *   │ MSR_SFMASK (0xC0000084) — SYSCALL Flag Mask                            │
 *   │   Bits set here are CLEARED in RFLAGS on SYSCALL entry:                │
 *   │     IF (bit 9):  Disable interrupts on entry                           │
 *   │     DF (bit 10): Clear direction flag (string ops go forward)          │
 *   │     TF (bit 8):  Disable single-stepping                               │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *
 * Syscall Flow:
 *
 *   Userspace                    Kernel
 *   ─────────                    ──────
 *   mov $SYS_WRITE, %rax
 *   mov fd, %rdi
 *   mov buf, %rsi
 *   mov count, %rdx
 *   syscall ─────────────────────┐
 *                                │
 *                                ▼
 *                          syscall_entry (entry.s)
 *                                │
 *                                │ 1. swapgs (get per-CPU data)
 *                                │ 2. Save user RSP, load kernel RSP
 *                                │ 3. Push all registers (SyscallFrame)
 *                                │ 4. Call syscall_dispatcher
 *                                │
 *                                ▼
 *                          syscall_dispatcher (this file)
 *                                │
 *                                │ Switch on RAX (syscall number)
 *                                │ Call sys_read/sys_write/etc.
 *                                │ Return result in RAX
 *                                │
 *                                ▼
 *                          syscall_entry (return path)
 *                                │
 *                                │ 1. Pop registers (except RAX = result)
 *                                │ 2. Restore user RSP
 *                                │ 3. swapgs (restore user GS)
 *                                │ 4. sysretq
 *                                │
 *   ◀────────────────────────────┘
 *   (RAX = return value)
 *
 * Syscall Numbers:
 *
 *   We use Linux syscall numbers where possible for familiarity. This doesn't
 *   provide binary compatibility (we'd need matching semantics), but it makes
 *   the interface recognizable to anyone who knows Linux.
 *
 * @see syscall_entry.s for the assembly entry point
 * @see percpu.hpp for per-CPU data structure used for stack switching
 */

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

/**
 * @brief Routes syscalls to their implementations based on syscall number.
 * @param frame Pointer to saved registers on kernel stack (built by syscall_entry.s).
 * @return Syscall result (placed in RAX by syscall_entry.s before sysretq).
 *
 * Called from syscall_entry.s after registers are saved. The syscall number is
 * in frame->rax, arguments in frame->rdi, rsi, rdx (matching System V ABI).
 */
extern "C"
std::uint64_t syscall_dispatcher(x86_64::entry::SyscallFrame* frame) {
    using namespace x86_64::entry;

    auto* process = x86_64::percpu::current_process();

    // Track context state for scheduler (see INSIGHTS.md "Why the scheduler
    // must check CS before context switching")
    process->has_kernel_context = true;
    process->has_user_context = false;

    const std::uint64_t syscall_num = frame->rax;
    const std::uint64_t arg1 = frame->rdi;
    const std::uint64_t arg2 = frame->rsi;
    const std::uint64_t arg3 = frame->rdx;

    switch (syscall_num) {
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
    case SYS_EXIT:
        return syscall::sys_exit(arg1);
    default:
        log::error("Unsupported syscall: ", frame->rax);
        return -ENOSYS;
    }
}

namespace x86_64::entry {
    /**
     * @brief Configures the CPU for SYSCALL/SYSRET operation.
     *
     * Programs the four MSRs that control SYSCALL behavior (see file header for
     * detailed MSR documentation). Must be called once during boot, before any
     * userspace code runs.
     */
    void init() {
        log::init_start("Syscall");

        // STAR MSR encodes segment selectors for both SYSCALL and SYSRET:
        //   [63:48] = 0x10: SYSRET base. CPU computes CS=base+16=0x20, SS=base+8=0x18
        //   [47:32] = 0x08: SYSCALL target. CS=0x08 (kernel code), SS=CS+8=0x10 (kernel data)
        // Note: SYSRET adds 16/8 and ORs with 3 for RPL, so user gets CS=0x23, SS=0x1B
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
