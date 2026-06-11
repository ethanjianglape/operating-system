# =============================================================================
# GDT Loading Routines
# =============================================================================
#
# After gdt.cpp populates the GDT structures in memory, these routines tell
# the CPU where to find them. This is the "hand it to the CPU" step.
#
# Why assembly? These operations require specific CPU instructions (LGDT, LTR)
# that have no C++ equivalent. We also need precise control over segment
# register loading, which compilers can't generate for us.
#
# =============================================================================

.code64

# Segment selectors - must match the GdtTable struct layout in gdt.hpp
# Each GDT entry is 8 bytes, so selector = index * 8
.set KERNEL_CODE_SEG, 0x08    # GDT[1]
.set KERNEL_DATA_SEG, 0x10    # GDT[2]
.set TSS_SEG, 0x28            # GDT[5]

# =============================================================================
# load_gdt - Load the Global Descriptor Table
# =============================================================================
# Input: RDI = pointer to GDTR structure (limit + base address)
#
# This function:
#   1. Loads the GDT register with LGDT instruction
#   2. Reloads all data segment registers (DS, ES, FS, GS, SS)
#   3. Reloads the code segment register (CS) via a far return trick
#
# After LGDT, the CPU knows where the GDT is, but the segment registers still
# hold old values. We must reload them to use our new GDT entries.
# =============================================================================
.global load_gdt
.type load_gdt, @function
load_gdt:
    # Tell the CPU where our GDT lives
    lgdt (%rdi)

    # Reload data segment registers with kernel data selector
    # In 64-bit mode these are mostly ignored, but SS is still checked
    # for privilege level, and some instructions use FS/GS for thread-local storage
    mov $KERNEL_DATA_SEG, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Reload CS register - this requires a trick:
    # There's no "mov %ax, %cs" instruction. The only ways to change CS are:
    #   - Far jump (jmp far)
    #   - Far call (call far)
    #   - Far return (retfq)
    #   - Interrupt return (iretq)
    #
    # We use the far return trick: push the new CS and return address onto
    # the stack, then execute retfq which pops both and jumps there.
    #
    # Stack before retfq:
    #   RSP+8  -> KERNEL_CODE_SEG (new CS value)
    #   RSP    -> address of .reload_cs (new RIP value)
    #
    # retfq pops RIP, then pops CS, effectively doing: CS:RIP = 0x08:.reload_cs
    lea .reload_cs(%rip), %rax
    push $KERNEL_CODE_SEG
    push %rax
    retfq

.reload_cs:
    # Now running with new CS! Just return to caller.
    ret

# =============================================================================
# load_tss - Load the Task State Segment
# =============================================================================
# The TSS tells the CPU where to find the kernel stack when transitioning
# from Ring 3 to Ring 0 (e.g., on syscall or interrupt from userspace).
#
# LTR (Load Task Register) points the CPU to our TSS descriptor in the GDT.
# Like LGDT, this is a one-time setup - we never call LTR again.
# =============================================================================
.global load_tss
.type load_tss, @function
load_tss:
    mov $TSS_SEG, %ax
    ltr %ax
    ret
