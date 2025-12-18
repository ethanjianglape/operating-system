# Multiboot2 header for GRUB bootloader
# This allows the kernel to be loaded by GRUB

.section .multiboot, "a"
.align 8
multiboot_start:
    .long 0xe85250d6                # Multiboot2 magic number
    .long 0                         # Architecture: i386 (0) - Multiboot2 spec for 32-bit x86
    .long multiboot_end - multiboot_start  # Header length
    .long 0x100000000 - (0xe85250d6 + 0 + (multiboot_end - multiboot_start)) # Checksum

    # Framebuffer tag
    .align 8
    .word 5    # MULTIBOOT_HEADER_TAG_FRAMEBUFFER
    .word 0    # flags
    .long 20   # tag size
    .long 1024 # screen width
    .long 768  # screen height
    .long 32   # bpp

    # End tag
    .align 8
    .word 0    # type
    .word 0    # flags
    .long 8    # size
multiboot_end:

.section .boot.data, "aw"
.align 4
multiboot_magic:    
    .long 0
multiboot_header_addr:
    .long 0

.align 4096
boot_page_directory:
    .skip 4096
boot_page_table:
    .skip 4096

.align 8
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

.section .boot.text, "ax"
.code32
.global _start
.extern kernel_main

_start:
    # Save multiboot info pointer (ebx contains multiboot info from GRUB)
    movl %eax, multiboot_magic
    movl %ebx, multiboot_header_addr

    # Verify we were loaded by a multiboot2 bootloader
    call check_multiboot

    # Load our own GDT (GRUB's GDT is minimal and may be overwritten)
    # This GDT is just temporary and will be replaced after we enter the kernel
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

    call enable_paging

    lea higher_half, %eax
    jmp *%eax

    # hang if kernel_main ever returns
.hang:
    hlt
    jmp .hang
.end:

higher_half:
    movl $stack_top, %esp

    pushl multiboot_header_addr
    pushl multiboot_magic

    call kernel_main

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

# Map boot_page_table to identity (0x00000000) and higher-half (0xC0000000)
# Entry 0: identity map first 4MB
# Entry 768: higher-half map (768 * 4MB = 3GB = 0xC0000000)
enable_paging:
    movl $boot_page_table, %edi
    movl $0x00000003, %esi  # Physical addr 0, present+writable
    movl $1024, %ecx        # 1024 entries = 4MB

fill_page_table:
    movl %esi, (%edi)
    addl $4096, %esi        # Next page (4KB)
    addl $4, %edi           # Next entry (4 bytes)
    loop fill_page_table

    movl $boot_page_directory, %edi
    movl $boot_page_table, %eax
    orl $0x03, %eax

    # PDE[0] = identity map (0x00000000)
    movl %eax, (%edi)

    # PDE[768] = higher-half map (0xC0000000)
    # 768 = 0xC0000000 >> 22
    movl %eax, 768*4(%edi)  # 768 * 4MB = 3GB = 0xC0000000

    # Load page table into cr3
    movl $boot_page_directory, %eax
    movl %eax, %cr3

    # Enable paging by setting bit 31 of cr0
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    ret
