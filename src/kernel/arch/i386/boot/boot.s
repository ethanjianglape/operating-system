# Multiboot2 header for GRUB bootloader
# This allows the kernel to be loaded by GRUB
.section .multiboot
.align 8
multiboot_start:
    .long 0xe85250d6                # Multiboot2 magic number
    .long 0                         # Architecture: i386 (0)
    .long multiboot_end - multiboot_start  # Header length
    .long 0x100000000 - (0xe85250d6 + 0 + (multiboot_end - multiboot_start)) # Checksum

    # End tag
    .word 0    # type
    .word 0    # flags
    .long 8    # size
multiboot_end:

.section .rodata

.section .text
.code32
.global _start
.extern kernel_main

_start:
    # GRUB has already put us in 32-bit protected mode
    # Set up stack
    movl $stack_top, %esp

    # Save multiboot info pointer (ebx contains multiboot info from GRUB)
    movl %ebx, %edi

    # Verify we were loaded by a multiboot2 bootloader
    call check_multiboot

    # Load our own GDT (GRUB's GDT is minimal and may be overwritten)
    cli
    lgdt gdt32_ptr

    # Far jump to reload CS with our code selector (0x08)
    # This also ensures we're using our GDT
    ljmp $0x08, $reload_segments

reload_segments:
    # Set all data segment registers to our data selector (0x10)
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Call the kernel main function
    call kernel_main

    # hang if kernel_main ever returns
.hang:
    hlt
    jmp .hang
.end:

# Prints `ERR: ` and the given error code to screen and hangs.
# parameter: error code (in ascii) in al
error:
    movl $0x4f524f45, 0xb8000
    movl $0x4f3a4f52, 0xb8004
    movl $0x4f204f20, 0xb8008
    movb %al, 0xb800a
    hlt

check_multiboot:
    cmpl $0x36d76289, %eax
    jne .no_multiboot
    ret
.no_multiboot:
    movb $'0', %al
    jmp error

check_cpuid:
    # Check if CPUID is supported by attempting to flip the ID bit (bit 21)
    # in the FLAGS register. If we can flip it, CPUID is available.

    # Copy FLAGS in to EAX via stack
    pushfl
    popl %eax

    # Copy to ECX as well for comparing later on
    movl %eax, %ecx

    # Flip the ID bit
    xorl $(1 << 21), %eax

    # Copy EAX to FLAGS via the stack
    pushl %eax
    popfl

    # Copy FLAGS back to EAX (with the flipped bit if CPUID is supported)
    pushfl
    popl %eax

    # Restore FLAGS from the old version stored in ECX (i.e. flipping the
    # ID bit back if it was ever flipped).
    pushl %ecx
    popfl

    # Compare EAX and ECX. If they are equal then that means the bit
    # wasn't flipped, and CPUID isn't supported.
    cmpl %ecx, %eax
    je .no_cpuid
    ret
.no_cpuid:
    movb $'1', %al
    jmp error

check_long_mode:
    # test if extended processor info in available
    movl $0x80000000, %eax    # implicit argument for cpuid
    cpuid                     # get highest supported argument
    cmpl $0x80000001, %eax    # it needs to be at least 0x80000001
    jb .no_long_mode          # if its less, the CPU is too old for long mode

    # use extended info to test if long mode is available
    movl $0x80000001, %eax    # argument for extended processor info
    cpuid                     # returns various feature bits in ecx and edx
    testl $(1 << 29), %edx    # test if the LM-bit is set in the D-register
    jz .no_long_mode          # If its not set, there is no long mode
    ret
.no_long_mode:
    movb $'2', %al
    jmp error

setup_page_tables:
    movl $p3_table, %eax
    orl $0b11, %eax
    movl %eax, p4_table

    movl $p2_table, %eax
    orl $0b11, %eax
    movl %eax, p3_table

    movl $0, %ecx

.map_p2_table:
    movl $0x200000, %eax # 2MiB
    mull %ecx
    orl $0b10000011, %eax  # present, writable, huge
    movl %eax, p2_table(,%ecx,8)

    incl %ecx
    cmpl $512, %ecx
    jne .map_p2_table

    ret

enable_paging:
    # CPU looks for page table in cr3 register
    movl $p4_table, %eax
    movl %eax, %cr3

    # Enable PAE-flag in cr4 (Physical Address Extension)
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4

    # Set the long mode bit in the EFER MSR (Model Specific Register)
    movl $0xC0000080, %ecx
    rdmsr
    orl $(1 << 8), %eax
    wrmsr

    movl %cr0, %eax
    orl $(1 << 31), %eax
    movl %eax, %cr0

    ret

gdt32:
    # gdt[0] null entry, never used
    .long 0
    .long 0

    # gdt[1] executable, read-only code, base address 0, limit of 0xFFFFF
    # 11111111 11111111 00000000 00000000 00000000 10011010 11001111 00000000
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0b10011010
    .byte 0b11001111
    .byte 0x00

    #gdt[2] writable data segment
    # 11111111 11111111 00000000 00000000 00000000 10010010 11001111 00000000
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0b10010010
    .byte 0b11001111
    .byte 0x00

gdt32_ptr:
    .word . - gdt32 - 1 # length of gdt32 section
    .long gdt32


.section .bss
.align 4096
p4_table:
    .skip 4096
p3_table:
    .skip 4096
p2_table:
    .skip 4096
stack_bottom:
    .skip 4096 * 4  # 16 KB stack
stack_top:
