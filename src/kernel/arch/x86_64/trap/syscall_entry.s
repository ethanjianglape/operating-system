# =============================================================================
# syscall_entry.s — SYSCALL instruction entry point
# =============================================================================
#
# This is where userspace enters the kernel via the SYSCALL instruction.
# SYSCALL is the modern, fast way to make system calls on x86-64 (vs the
# legacy INT 0x80 software interrupt method).
#
# What SYSCALL Does (hardware):
#
#   When userspace executes SYSCALL, the CPU:
#     1. Saves RIP to RCX (return address)
#     2. Saves RFLAGS to R11
#     3. Loads CS from STAR[47:32], SS from STAR[47:32]+8
#     4. Loads RIP from LSTAR (points here!)
#     5. Masks RFLAGS with SFMASK (we clear IF to disable interrupts)
#
#   Notably, SYSCALL does NOT:
#     - Switch stacks (RSP is still the user stack!)
#     - Save any other registers
#     - Push anything to any stack
#
#   This is why our first instructions must be swapgs + stack switch.
#   We're running kernel code but still on the user stack — very dangerous.
#
# The Stack Switch Problem:
#
#   We need to switch to a kernel stack, but we can't use any registers yet
#   (they all contain user values we need to save). The solution:
#
#     1. swapgs — Now GS points to per-CPU data (see percpu.cpp)
#     2. Save user RSP to %gs:PER_CPU_USER_RSP_OFFSET
#     3. Load kernel RSP from %gs:PER_CPU_KERNEL_RSP_OFFSET
#
#   Now we have a safe stack and can push registers normally.
#
# SyscallFrame Layout:
#
#   After pushing all registers, the stack matches SyscallFrame in syscall_entry.hpp:
#
#       +0x00  ss      (we push ss/cs for debugging/consistency)
#       +0x08  cs
#       +0x10  r15
#       +0x18  r14
#       ...
#       +0x78  rbx
#       +0x80  rax     <- syscall number on entry, return value on exit
#
#   We pass RSP (pointing to this frame) to syscall_dispatcher(), which
#   reads arguments from the frame and returns the result in RAX.
#
# Return Path:
#
#   After syscall_dispatcher returns:
#     1. Pop all registers EXCEPT rax (it has the return value)
#     2. Restore user RSP from per-CPU data
#     3. swapgs to restore user GS
#     4. sysretq — returns to userspace
#
#   SYSRETQ loads RIP from RCX, RFLAGS from R11 (saved by SYSCALL hardware),
#   and sets CS/SS for ring 3. It's the inverse of SYSCALL.
#
# =============================================================================

.code64

.extern syscall_dispatcher

# =============================================================================
# Per-CPU structure offsets (must match PerCPU in percpu.hpp!)
# =============================================================================

.set PER_CPU_SELF_OFFSET,       0
.set PER_CPU_KERNEL_RSP_OFFSET, 8
.set PER_CPU_USER_RSP_OFFSET,   16
.set PER_CPU_PROCESS_OFFSET,    24

# =============================================================================
# syscall_entry — Main SYSCALL handler
# =============================================================================

.global syscall_entry
syscall_entry:
    # CRITICAL: We're on the user stack with user GS. Fix both immediately.
    swapgs
    mov %rsp, %gs:PER_CPU_USER_RSP_OFFSET   # Save user stack pointer
    mov %gs:PER_CPU_KERNEL_RSP_OFFSET, %rsp # Switch to kernel stack

    # Now safe to use the stack. Build SyscallFrame for syscall_dispatcher.
    # Push order must match SyscallFrame struct (reversed since stack grows down).
    push %rax       # Syscall number (will be overwritten with return value)
    push %rbx
    push %rcx       # Contains user RIP (saved by SYSCALL hardware)
    push %rdx       # Arg 3
    push %rsi       # Arg 2
    push %rdi       # Arg 1
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11       # Contains user RFLAGS (saved by SYSCALL hardware)
    push %r12
    push %r13
    push %r14
    push %r15

    # Push CS/SS for SyscallFrame completeness (useful for debugging)
    mov %cs, %r15
    push %r15
    mov %ss, %r15
    push %r15

    # Call C++ dispatcher: syscall_dispatcher(SyscallFrame* frame)
    mov %rsp, %rdi  # First argument = pointer to SyscallFrame
    call syscall_dispatcher
    # Return value is now in RAX

    # Restore registers (skip ss/cs, they're not real saved state)
    pop %r15        # Discard ss
    pop %r15        # Discard cs
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx

    # Skip popping RAX — it contains the syscall return value from dispatcher
    add $8, %rsp

    # Restore user stack and GS, then return to userspace
    mov %gs:PER_CPU_USER_RSP_OFFSET, %rsp
    swapgs
    sysretq
