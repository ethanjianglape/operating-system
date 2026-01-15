.code64

.extern syscall_dispatcher
//.extern PER_CPU_KERNEL_RSP_OFFSET
//.extern PER_CPU_USER_RSP_OFFSET

.set PER_CPU_SELF_OFFSET, 0
.set PER_CPU_KERNEL_RSP_OFFSET, 8
.set PER_CPU_USER_RSP_OFFSET, 16
.set PER_CPU_PROCESS_OFFSET, 24

#.set PER_CPU_KERNEL_RSP_OFFSET, 0
#.set PER_CPU_USER_RSP_OFFSET, 8

.global syscall_entry
syscall_entry:
    swapgs
    mov %rsp, %gs:PER_CPU_USER_RSP_OFFSET
    mov %gs:PER_CPU_KERNEL_RSP_OFFSET, %rsp

    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %cs, %r15  
    push %r15
    mov %ss, %r15
    push %r15

    mov %rsp, %rdi
    call syscall_dispatcher
    
    pop %r15
    pop %r15
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

    # rax contains return value of syscall_dispatcher, instead of popping rax,
    # add 8 to the stack to skip it
    add $8, %rsp

    mov %gs:PER_CPU_USER_RSP_OFFSET, %rsp
    swapgs
    sysretq

