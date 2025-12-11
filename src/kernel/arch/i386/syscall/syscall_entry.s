.code32
.global syscall_entry

.extern syscall_dispatcher

# Struct layout on stack (pushed in reverse order):
# offset 0:  EDI
# offset 4:  ESI  
# offset 8:  EBP
# offset 12: ESP (ignored, will use value after popa)
# offset 16: EBX
# offset 20: EDX
# offset 24: ECX
# offset 28: EAX (syscall number, also return value)

syscall_entry:
    pusha
    push %esp
    call syscall_dispatcher
    add $4, %esp
    mov %eax, 28(%esp)
    popa
    iret
