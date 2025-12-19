.code64

.set KERNEL_CODE_SEG, 0x08
.set KERNEL_DATA_SEG, 0x10
.set TSS_SEG, 0x28

.global load_gdt
.type load_gdt, @function
load_gdt:
    lgdt (%rdi)

    mov $KERNEL_DATA_SEG, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    lea .reload_cs(%rip), %rax
    push $KERNEL_CODE_SEG
    push %rax
    retfq

.reload_cs:
    ret

.global load_tss
.type load_tss, @function
load_tss:
    mov $TSS_SEG, %ax
    ltr %ax
    ret
