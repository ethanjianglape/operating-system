.code64

.global context_switch
.type context_switch, @function
context_switch:
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, (%rdi)

    mov %rsi, %rsp

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp

    ret

.global userspace_entry_trampoline
userspace_entry_trampoline:
    # Swap GS back to user mode before returning to userspace
    # (syscall_entry did swapgs to get kernel GS, we need to undo it)
    cli
    swapgs

    # Build iretq frame: SS, RSP, RFLAGS, CS, RIP
    push $0x1B          # SS = user data
    push %r14           # RSP = user stack (from ContextFrame)
    push $0x202         # RFLAGS (IF=1)
    push $0x23          # CS = user code
    push %r15           # RIP = user entry point (from ContextFrame)

    iretq
