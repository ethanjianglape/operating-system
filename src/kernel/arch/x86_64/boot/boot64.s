# x86_64 Long Mode Boot
# Multiboot2 compliant bootloader that transitions from 32-bit protected mode to 64-bit long mode

# ============================================================================
# Multiboot2 Header
# ============================================================================
.section .multiboot, "a"
.align 8
multiboot_start:
    .long 0xe85250d6                                    # Multiboot2 magic
    .long 0                                             # Architecture: i386 (32-bit entry)
    .long multiboot_end - multiboot_start               # Header length
    .long -(0xe85250d6 + 0 + (multiboot_end - multiboot_start))  # Checksum

    # Framebuffer tag
    .align 8
    .word 5                                             # MULTIBOOT_HEADER_TAG_FRAMEBUFFER
    .word 0                                             # flags
    .long 20                                            # tag size
    .long 1024                                          # width
    .long 768                                           # height
    .long 32                                            # bpp

    # End tag
    .align 8
    .word 0                                             # type
    .word 0                                             # flags
    .long 8                                             # size
multiboot_end:

# ============================================================================
# Boot Data (32-bit accessible)
# ============================================================================
.section .boot.data, "aw"

# Save multiboot info for later
.align 8
multiboot_magic:
    .long 0
multiboot_info_addr:
    .long 0

# -----------------------------------------------------------------------------
# 4-Level Page Tables for Long Mode
# Each table: 512 entries × 8 bytes = 4096 bytes
#
# Memory map:
#   Identity map:     0x0000000000000000 → physical 0x0 (first 4MB)
#   Higher-half:      0xFFFFFFFF80000000 → physical 0x0 (first 4MB)
#
# Higher-half breakdown (0xFFFFFFFF80000000):
#   PML4 index:  511 (bits 47:39)
#   PDPT index:  510 (bits 38:30)
#   PD index:    0   (bits 29:21)
# -----------------------------------------------------------------------------

.align 4096
boot_pml4:
    .skip 4096

.align 4096
boot_pdpt_low:      # For identity mapping (PML4[0])
    .skip 4096

.align 4096
boot_pdpt_high:     # For higher-half mapping (PML4[511])
    .skip 4096

.align 4096
boot_pd:            # Shared PD with 2MB pages
    .skip 4096

# -----------------------------------------------------------------------------
# 64-bit GDT
# -----------------------------------------------------------------------------
.align 16
gdt64:
    # Null descriptor (required)
    .quad 0

    # Code segment (selector 0x08)
    # Access: Present=1, DPL=0, Type=Code Execute/Read
    # Flags: Long=1, Size=0 (must be 0 for 64-bit)
    .quad 0x00AF9A000000FFFF

    # Data segment (selector 0x10)
    # Access: Present=1, DPL=0, Type=Data Read/Write
    .quad 0x00CF92000000FFFF

gdt64_end:

gdt64_ptr:
    .word gdt64_end - gdt64 - 1     # Limit (size - 1)
    .quad gdt64                      # Base address (8 bytes for 64-bit)

# ============================================================================
# 32-bit Boot Code
# ============================================================================
.section .boot.text, "ax"
.code32
.global _start
.extern kernel_main

_start:
    # Disable interrupts
    cli

    # Save multiboot info (eax = magic, ebx = info pointer)
    movl %eax, multiboot_magic
    movl %ebx, multiboot_info_addr

    # Verify multiboot2 magic
    cmpl $0x36d76289, %eax
    jne .error_no_multiboot

    # Check for CPUID support
    call check_cpuid

    # Check for long mode support
    call check_long_mode

    # Set up 4-level paging
    call setup_page_tables

    # Enable PAE (Physical Address Extension)
    movl %cr4, %eax
    orl $(1 << 5), %eax             # Set CR4.PAE
    movl %eax, %cr4

    # Load PML4 into CR3
    movl $boot_pml4, %eax
    movl %eax, %cr3

    # Enable long mode by setting EFER.LME
    movl $0xC0000080, %ecx          # EFER MSR
    rdmsr
    orl $(1 << 8), %eax             # Set LME (Long Mode Enable)
    wrmsr

    # Enable paging (this activates long mode)
    movl %cr0, %eax
    orl $(1 << 31), %eax            # Set CR0.PG
    movl %eax, %cr0

    # Load 64-bit GDT
    lgdt gdt64_ptr

    # Far jump to 64-bit code (this completes the switch to long mode)
    ljmp $0x08, $long_mode_start

# -----------------------------------------------------------------------------
# Check if CPUID is supported
# -----------------------------------------------------------------------------
check_cpuid:
    # Try to flip the ID bit (bit 21) in EFLAGS
    pushfl
    popl %eax
    movl %eax, %ecx                 # Save original
    xorl $(1 << 21), %eax           # Flip ID bit
    pushl %eax
    popfl
    pushfl
    popl %eax
    pushl %ecx                      # Restore original EFLAGS
    popfl
    cmpl %ecx, %eax                 # If equal, CPUID not supported
    je .error_no_cpuid
    ret

# -----------------------------------------------------------------------------
# Check if long mode is supported
# -----------------------------------------------------------------------------
check_long_mode:
    # Check if extended CPUID functions are available
    movl $0x80000000, %eax
    cpuid
    cmpl $0x80000001, %eax          # Need at least 0x80000001
    jb .error_no_long_mode

    # Check for long mode support
    movl $0x80000001, %eax
    cpuid
    testl $(1 << 29), %edx          # Test LM bit
    jz .error_no_long_mode
    ret

# -----------------------------------------------------------------------------
# Set up 4-level page tables with 2MB pages
# -----------------------------------------------------------------------------
setup_page_tables:
    # Clear all page tables first
    movl $boot_pml4, %edi
    xorl %eax, %eax
    movl $(4096 * 4 / 4), %ecx      # 4 tables, 4096 bytes each, 4 bytes per stosl
    rep stosl

    # PML4[0] → boot_pdpt_low (for identity mapping)
    movl $boot_pdpt_low, %eax
    orl $0x03, %eax                 # Present + Writable
    movl %eax, boot_pml4

    # PML4[511] → boot_pdpt_high (for higher-half at 0xFFFFFFFF80000000)
    movl $boot_pdpt_high, %eax
    orl $0x03, %eax
    movl %eax, boot_pml4 + 511 * 8

    # PDPT_low[0] → boot_pd
    movl $boot_pd, %eax
    orl $0x03, %eax
    movl %eax, boot_pdpt_low

    # PDPT_high[510] → boot_pd (510 because 0xFFFFFFFF80000000 has PDPT index 510)
    movl $boot_pd, %eax
    orl $0x03, %eax
    movl %eax, boot_pdpt_high + 510 * 8

    # Fill PD with 2MB pages (identity map first 32MB)
    # 16 entries x 2MB = 32MB
    movl $boot_pd, %edi
    movl $0x00000083, %eax
    movl $16, %ecx

.fill_pd:  
    movl %eax, (%edi)
    addl $8, %edi
    addl $0x200000, %eax
    decl %ecx
    jnz .fill_pd

    ret

# -----------------------------------------------------------------------------
# Error handlers
# -----------------------------------------------------------------------------
.error_no_multiboot:
    movb $'M', %al
    jmp .error

.error_no_cpuid:
    movb $'C', %al
    jmp .error

.error_no_long_mode:
    movb $'L', %al
    jmp .error

.error:
    # Print "ERR: X" to VGA buffer
    movl $0x4f524f45, 0xb8000       # "ER"
    movl $0x4f3a4f52, 0xb8004       # "R:"
    movl $0x4f204f20, 0xb8008       # "  "
    movb %al, 0xb800a               # Error code
    hlt
    jmp .error

# ============================================================================
# 64-bit Long Mode Code
# ============================================================================
.code64
long_mode_start:
    # Reload data segments with 64-bit data selector
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Set up 64-bit stack (use higher-half address)
    movabsq $stack_top, %rsp

    # Jump to higher-half address
    movabsq $higher_half_start, %rax
    jmp *%rax

higher_half_start:
    # Clear direction flag
    cld

    # Call kernel_main
    # In x86_64 System V ABI, first args go in rdi, rsi
    # For now, no arguments
    mov multiboot_magic, %rdi
    mov multiboot_info_addr, %rsi
    movabsq $kernel_main, %rax
    call *%rax

    # If kernel_main returns, halt
.halt:
    hlt
    jmp .halt
