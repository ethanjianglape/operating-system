# Multiboot2 header for GRUB bootloader
# This allows the kernel to be loaded by GRUB

.intel_syntax noprefix

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
#gdt64:
#    .quad 0
#.set gdt64_code, . - gdt64
#    .quad (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
#gdt64_pointer:
#    .word . - gdt64 - 1
#    .quad gdt64

.section .text
.code32
.global _start
.extern kernel_main
#.extern long_mode_start

_start:
    # Set up stack
    mov esp, offset stack_top

    # Copy multiboot info pointer to edi to use in kernel_main later on
    mov edi, ebx 

    call check_multiboot
    call check_cpuid
    call check_long_mode

    call setup_page_tables
    call enable_paging

    call kernel_main

    #lgdt [gdt64_pointer]

    # Far jump to 64-bit code
    #push gdt64_code
    #push offset long_mode_start
    #retf
.hang:
    hlt
    jmp .hang
.end:

# Prints `ERR: ` and the given error code to screen and hangs.
# parameter: error code (in ascii) in al
error:
    mov dword ptr [0xb8000], 0x4f524f45
    mov dword ptr [0xb8004], 0x4f3a4f52
    mov dword ptr [0xb8008], 0x4f204f20
    mov byte ptr  [0xb800a], al
    hlt

check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, '0'
    jmp error

check_cpuid:
    # Check if CPUID is supported by attempting to flip the ID bit (bit 21)
    # in the FLAGS register. If we can flip it, CPUID is available.

    # Copy FLAGS in to EAX via stack
    pushfd
    pop eax

    # Copy to ECX as well for comparing later on
    mov ecx, eax

    # Flip the ID bit
    xor eax, 1 << 21

    # Copy EAX to FLAGS via the stack
    push eax
    popfd

    # Copy FLAGS back to EAX (with the flipped bit if CPUID is supported)
    pushfd
    pop eax

    # Restore FLAGS from the old version stored in ECX (i.e. flipping the
    # ID bit back if it was ever flipped).
    push ecx
    popfd

    # Compare EAX and ECX. If they are equal then that means the bit
    # wasn't flipped, and CPUID isn't supported.
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, '1'
    jmp error

check_long_mode:
    # test if extended processor info in available
    mov eax, 0x80000000    # implicit argument for cpuid
    cpuid                  # get highest supported argument
    cmp eax, 0x80000001    # it needs to be at least 0x80000001
    jb .no_long_mode       # if its less, the CPU is too old for long mode

    # use extended info to test if long mode is available
    mov eax, 0x80000001    # argument for extended processor info
    cpuid                  # returns various feature bits in ecx and edx
    test edx, 1 << 29      # test if the LM-bit is set in the D-register
    jz .no_long_mode       # If its not set, there is no long mode
    ret
.no_long_mode:
    mov al, '2'
    jmp error

setup_page_tables:
    mov eax, offset p3_table
    or eax, 0b11
    mov [p4_table], eax

    mov eax, offset p2_table
    or eax, 0b11
    mov [p3_table], eax

    mov ecx, 0

.map_p2_table:
    mov eax, 0x200000 # 2MiB
    mul ecx
    or eax, 0b10000011  # present, writable, huge
    mov [p2_table + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .map_p2_table

    ret

enable_paging:
    # CPU looks for page table in cr3 register
    mov eax, offset p4_table
    mov cr3, eax

    # Enable PAE-flag in cr4 (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    # Set the long mode bit in the EFER MSR (Model Specific Register)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

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
