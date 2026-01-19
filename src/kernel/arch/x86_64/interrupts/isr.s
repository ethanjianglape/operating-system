# ============================================================================
# isr.s — Interrupt Service Routine (ISR) Stubs
# ============================================================================
#
# This is the code that ACTUALLY runs when a CPU interrupt occurs.
#
# The IDT (idt.cpp) is just a lookup table - it tells the CPU "when interrupt
# X happens, jump to address Y". This file contains all those addresses Y.
# Each of the 256 interrupt vectors needs its own entry point (stub) because
# the stub must push the vector number onto the stack so the C++ handler
# knows which interrupt occurred.
#
# ============================================================================
# Interrupt Flow
# ============================================================================
#
#   1. Interrupt occurs (hardware, exception, or INT instruction)
#   2. CPU looks up handler address in IDT[vector]
#   3. CPU pushes SS, RSP, RFLAGS, CS, RIP (and error code for some exceptions)
#   4. CPU jumps to the handler address (our stub)
#   5. Stub pushes vector number (and dummy error code if CPU didn't push one)
#   6. Stub jumps to isr_common
#   7. isr_common saves all registers, calls C++ interrupt_handler()
#   8. C++ handler processes the interrupt
#   9. isr_common restores registers and executes IRETQ
#  10. CPU pops RIP, CS, RFLAGS, RSP, SS and resumes interrupted code
#
# ============================================================================
# The Error Code Problem
# ============================================================================
#
# Some CPU exceptions push an error code onto the stack, others don't:
#
#   WITH error code (CPU pushes it):    WITHOUT error code (we push dummy 0):
#     0x08 - Double Fault                 0x00-0x07 - Various exceptions
#     0x0A - Invalid TSS                  0x09 - Coprocessor Segment Overrun
#     0x0B - Segment Not Present          0x0F - Reserved
#     0x0C - Stack Segment Fault          0x10 - x87 FPU Error
#     0x0D - General Protection Fault     0x12 - Machine Check
#     0x0E - Page Fault                   0x13 - SIMD Exception
#     0x11 - Alignment Check              0x20+ - All hardware IRQs
#     0x1E - Security Exception
#
# To keep the stack layout consistent for the C++ handler, we use two macros:
#   - isr_err_stub: For exceptions where CPU already pushed error code
#   - isr_no_err_stub: Pushes a dummy 0 as error code for consistency
#
# ============================================================================
# Stack Layout (when isr_common calls interrupt_handler)
# ============================================================================
#
# This is what the InterruptFrame struct in irq.hpp maps to:
#
#   ┌─────────────────────┐  High addresses
#   │        SS           │  ─┐
#   │        RSP          │   │ Pushed by CPU
#   │       RFLAGS        │   │ (always, on interrupt)
#   │        CS           │   │
#   │        RIP          │  ─┘
#   ├─────────────────────┤
#   │     Error Code      │  ← Pushed by CPU (some exceptions) or stub (dummy 0)
#   │    Vector Number    │  ← Pushed by stub
#   ├─────────────────────┤
#   │        RAX          │  ─┐
#   │        RBX          │   │
#   │        RCX          │   │
#   │        RDX          │   │
#   │        RSI          │   │
#   │        RDI          │   │ Pushed by isr_common
#   │        RBP          │   │ (general-purpose registers)
#   │        R8           │   │
#   │        R9           │   │
#   │        R10          │   │
#   │        R11          │   │
#   │        R12          │   │
#   │        R13          │   │
#   │        R14          │   │
#   │        R15          │  ─┘
#   └─────────────────────┘  Low addresses (RSP points here)
#                            │
#                            └─► Passed to interrupt_handler(InterruptFrame* frame)
#
# ============================================================================
# Why Assembly?
# ============================================================================
#
# This must be assembly because:
#   1. We need exact control over the stack layout
#   2. We must save ALL registers before calling C++ (ABI only preserves some)
#   3. We need IRETQ instruction to return from interrupt (no C++ equivalent)
#   4. Each stub must be a separate symbol so IDT can point to it
#
# ============================================================================
# Why 256 Separate Stubs?
# ============================================================================
#
# You might wonder: "Why not just have one handler that reads the vector
# number from somewhere?" The problem is: when the CPU jumps to our handler,
# it doesn't tell us WHICH interrupt occurred - we have to already know based
# on which handler was called. So each vector needs its own stub that pushes
# its own vector number.
#
# We use macros to generate these 256 stubs to avoid writing them all by hand.
# At the bottom of this file, isr_stub_table is an array of 256 pointers to
# these stubs, which idt.cpp uses to populate the IDT.
#
# ============================================================================

.code64
.global isr_stub_table

.extern interrupt_handler

# ============================================================================
# Stub Macros
# ============================================================================
#
# isr_err_stub: For exceptions where CPU pushes an error code
#   Stack on entry: [RIP] [CS] [RFLAGS] [RSP] [SS] [ERROR]
#   We just push the vector number and jump to common handler
#
# isr_no_err_stub: For everything else (no CPU error code)
#   Stack on entry: [RIP] [CS] [RFLAGS] [RSP] [SS]
#   We push a dummy 0 as error code, then vector number, for consistent layout

.macro isr_err_stub num
isr_stub_\num:
    pushq $\num
    jmp isr_common
.endm

.macro isr_no_err_stub num
isr_stub_\num:
    pushq $0
    pushq $\num
    jmp isr_common
.endm

# ============================================================================
# Common Interrupt Handler
# ============================================================================
#
# All stubs jump here. We save the full register state, call the C++ handler,
# restore registers, and return from interrupt.

isr_common:
    # Check if we came from kernel (CS=0x08) or user mode, swap GS only if user
    cmp $0x08, 24(%rsp)
    je skip_swapgs_entry
    swapgs

skip_swapgs_entry:
    # Save all general-purpose registers (in reverse order of InterruptFrame)
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

    # RSP now points to our complete InterruptFrame struct
    # Pass it as first argument to interrupt_handler (System V ABI: RDI = arg1)
    mov %rsp, %rdi
    call interrupt_handler

    # Restore all general-purpose registers
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
    pop %rax

    add $16, %rsp           # Remove vector number and error code (16 bytes)

    # Swap GS back if we came from user mode
    cmp $0x08, 8(%rsp)
    je skip_swapgs_exit
    swapgs

skip_swapgs_exit:
    iretq                   # Return from interrupt

# ============================================================================
# ISR Stubs
# ============================================================================
#
# These define the functions that will be used for every IDT vector (0-255).
# isr_no_err_stub and isr_err_stub are just compiler macros, so when compiled
# these will be converted into actual callable functions, the macro is just
# used to avoid repetative typing

# ============================================================================
# Exception Stubs (Vectors 0x00 - 0x1F)
# ============================================================================
#
# CPU exceptions. Some push error codes (use isr_err_stub), others don't.
isr_no_err_stub 0x00
isr_no_err_stub 0x01
isr_no_err_stub 0x02
isr_no_err_stub 0x03
isr_no_err_stub 0x04
isr_no_err_stub 0x05
isr_no_err_stub 0x06
isr_no_err_stub 0x07
isr_err_stub    0x08
isr_no_err_stub 0x09
isr_err_stub    0x0a
isr_err_stub    0x0b
isr_err_stub    0x0c
isr_err_stub    0x0d
isr_err_stub    0x0e
isr_no_err_stub 0x0f
isr_no_err_stub 0x10
isr_err_stub    0x11
isr_no_err_stub 0x12
isr_no_err_stub 0x13
isr_no_err_stub 0x14
isr_no_err_stub 0x15
isr_no_err_stub 0x16
isr_no_err_stub 0x17
isr_no_err_stub 0x18
isr_no_err_stub 0x19
isr_no_err_stub 0x1a
isr_no_err_stub 0x1b
isr_no_err_stub 0x1c
isr_no_err_stub 0x1d
isr_err_stub    0x1e
isr_no_err_stub 0x1f

# ============================================================================
# IRQ Stubs (Vectors 0x20 - 0xFF)
# ============================================================================
#
# Hardware IRQs and software interrupts. None of these have CPU error codes.

isr_no_err_stub 0x20
isr_no_err_stub 0x21
isr_no_err_stub 0x22
isr_no_err_stub 0x23
isr_no_err_stub 0x24
isr_no_err_stub 0x25
isr_no_err_stub 0x26
isr_no_err_stub 0x27
isr_no_err_stub 0x28
isr_no_err_stub 0x29
isr_no_err_stub 0x2a
isr_no_err_stub 0x2b
isr_no_err_stub 0x2c
isr_no_err_stub 0x2d
isr_no_err_stub 0x2e
isr_no_err_stub 0x2f
isr_no_err_stub 0x30
isr_no_err_stub 0x31
isr_no_err_stub 0x32
isr_no_err_stub 0x33
isr_no_err_stub 0x34
isr_no_err_stub 0x35
isr_no_err_stub 0x36
isr_no_err_stub 0x37
isr_no_err_stub 0x38
isr_no_err_stub 0x39
isr_no_err_stub 0x3a
isr_no_err_stub 0x3b
isr_no_err_stub 0x3c
isr_no_err_stub 0x3d
isr_no_err_stub 0x3e
isr_no_err_stub 0x3f
isr_no_err_stub 0x40
isr_no_err_stub 0x41
isr_no_err_stub 0x42
isr_no_err_stub 0x43
isr_no_err_stub 0x44
isr_no_err_stub 0x45
isr_no_err_stub 0x46
isr_no_err_stub 0x47
isr_no_err_stub 0x48
isr_no_err_stub 0x49
isr_no_err_stub 0x4a
isr_no_err_stub 0x4b
isr_no_err_stub 0x4c
isr_no_err_stub 0x4d
isr_no_err_stub 0x4e
isr_no_err_stub 0x4f
isr_no_err_stub 0x50
isr_no_err_stub 0x51
isr_no_err_stub 0x52
isr_no_err_stub 0x53
isr_no_err_stub 0x54
isr_no_err_stub 0x55
isr_no_err_stub 0x56
isr_no_err_stub 0x57
isr_no_err_stub 0x58
isr_no_err_stub 0x59
isr_no_err_stub 0x5a
isr_no_err_stub 0x5b
isr_no_err_stub 0x5c
isr_no_err_stub 0x5d
isr_no_err_stub 0x5e
isr_no_err_stub 0x5f
isr_no_err_stub 0x60
isr_no_err_stub 0x61
isr_no_err_stub 0x62
isr_no_err_stub 0x63
isr_no_err_stub 0x64
isr_no_err_stub 0x65
isr_no_err_stub 0x66
isr_no_err_stub 0x67
isr_no_err_stub 0x68
isr_no_err_stub 0x69
isr_no_err_stub 0x6a
isr_no_err_stub 0x6b
isr_no_err_stub 0x6c
isr_no_err_stub 0x6d
isr_no_err_stub 0x6e
isr_no_err_stub 0x6f
isr_no_err_stub 0x70
isr_no_err_stub 0x71
isr_no_err_stub 0x72
isr_no_err_stub 0x73
isr_no_err_stub 0x74
isr_no_err_stub 0x75
isr_no_err_stub 0x76
isr_no_err_stub 0x77
isr_no_err_stub 0x78
isr_no_err_stub 0x79
isr_no_err_stub 0x7a
isr_no_err_stub 0x7b
isr_no_err_stub 0x7c
isr_no_err_stub 0x7d
isr_no_err_stub 0x7e
isr_no_err_stub 0x7f
isr_no_err_stub 0x80
isr_no_err_stub 0x81
isr_no_err_stub 0x82
isr_no_err_stub 0x83
isr_no_err_stub 0x84
isr_no_err_stub 0x85
isr_no_err_stub 0x86
isr_no_err_stub 0x87
isr_no_err_stub 0x88
isr_no_err_stub 0x89
isr_no_err_stub 0x8a
isr_no_err_stub 0x8b
isr_no_err_stub 0x8c
isr_no_err_stub 0x8d
isr_no_err_stub 0x8e
isr_no_err_stub 0x8f
isr_no_err_stub 0x90
isr_no_err_stub 0x91
isr_no_err_stub 0x92
isr_no_err_stub 0x93
isr_no_err_stub 0x94
isr_no_err_stub 0x95
isr_no_err_stub 0x96
isr_no_err_stub 0x97
isr_no_err_stub 0x98
isr_no_err_stub 0x99
isr_no_err_stub 0x9a
isr_no_err_stub 0x9b
isr_no_err_stub 0x9c
isr_no_err_stub 0x9d
isr_no_err_stub 0x9e
isr_no_err_stub 0x9f
isr_no_err_stub 0xa0
isr_no_err_stub 0xa1
isr_no_err_stub 0xa2
isr_no_err_stub 0xa3
isr_no_err_stub 0xa4
isr_no_err_stub 0xa5
isr_no_err_stub 0xa6
isr_no_err_stub 0xa7
isr_no_err_stub 0xa8
isr_no_err_stub 0xa9
isr_no_err_stub 0xaa
isr_no_err_stub 0xab
isr_no_err_stub 0xac
isr_no_err_stub 0xad
isr_no_err_stub 0xae
isr_no_err_stub 0xaf
isr_no_err_stub 0xb0
isr_no_err_stub 0xb1
isr_no_err_stub 0xb2
isr_no_err_stub 0xb3
isr_no_err_stub 0xb4
isr_no_err_stub 0xb5
isr_no_err_stub 0xb6
isr_no_err_stub 0xb7
isr_no_err_stub 0xb8
isr_no_err_stub 0xb9
isr_no_err_stub 0xba
isr_no_err_stub 0xbb
isr_no_err_stub 0xbc
isr_no_err_stub 0xbd
isr_no_err_stub 0xbe
isr_no_err_stub 0xbf
isr_no_err_stub 0xc0
isr_no_err_stub 0xc1
isr_no_err_stub 0xc2
isr_no_err_stub 0xc3
isr_no_err_stub 0xc4
isr_no_err_stub 0xc5
isr_no_err_stub 0xc6
isr_no_err_stub 0xc7
isr_no_err_stub 0xc8
isr_no_err_stub 0xc9
isr_no_err_stub 0xca
isr_no_err_stub 0xcb
isr_no_err_stub 0xcc
isr_no_err_stub 0xcd
isr_no_err_stub 0xce
isr_no_err_stub 0xcf
isr_no_err_stub 0xd0
isr_no_err_stub 0xd1
isr_no_err_stub 0xd2
isr_no_err_stub 0xd3
isr_no_err_stub 0xd4
isr_no_err_stub 0xd5
isr_no_err_stub 0xd6
isr_no_err_stub 0xd7
isr_no_err_stub 0xd8
isr_no_err_stub 0xd9
isr_no_err_stub 0xda
isr_no_err_stub 0xdb
isr_no_err_stub 0xdc
isr_no_err_stub 0xdd
isr_no_err_stub 0xde
isr_no_err_stub 0xdf
isr_no_err_stub 0xe0
isr_no_err_stub 0xe1
isr_no_err_stub 0xe2
isr_no_err_stub 0xe3
isr_no_err_stub 0xe4
isr_no_err_stub 0xe5
isr_no_err_stub 0xe6
isr_no_err_stub 0xe7
isr_no_err_stub 0xe8
isr_no_err_stub 0xe9
isr_no_err_stub 0xea
isr_no_err_stub 0xeb
isr_no_err_stub 0xec
isr_no_err_stub 0xed
isr_no_err_stub 0xee
isr_no_err_stub 0xef
isr_no_err_stub 0xf0
isr_no_err_stub 0xf1
isr_no_err_stub 0xf2
isr_no_err_stub 0xf3
isr_no_err_stub 0xf4
isr_no_err_stub 0xf5
isr_no_err_stub 0xf6
isr_no_err_stub 0xf7
isr_no_err_stub 0xf8
isr_no_err_stub 0xf9
isr_no_err_stub 0xfa
isr_no_err_stub 0xfb
isr_no_err_stub 0xfc
isr_no_err_stub 0xfd
isr_no_err_stub 0xfe
isr_no_err_stub 0xff

# ============================================================================
# ISR Stub Table
# ============================================================================
#
# Array of 256 pointers to our stub functions. idt.cpp reads this table to
# populate the IDT entries. Each entry is a 64-bit address (.quad).

isr_stub_table:
    .quad isr_stub_0x00
    .quad isr_stub_0x01
    .quad isr_stub_0x02
    .quad isr_stub_0x03
    .quad isr_stub_0x04
    .quad isr_stub_0x05
    .quad isr_stub_0x06
    .quad isr_stub_0x07
    .quad isr_stub_0x08
    .quad isr_stub_0x09
    .quad isr_stub_0x0a
    .quad isr_stub_0x0b
    .quad isr_stub_0x0c
    .quad isr_stub_0x0d
    .quad isr_stub_0x0e
    .quad isr_stub_0x0f
    .quad isr_stub_0x10
    .quad isr_stub_0x11
    .quad isr_stub_0x12
    .quad isr_stub_0x13
    .quad isr_stub_0x14
    .quad isr_stub_0x15
    .quad isr_stub_0x16
    .quad isr_stub_0x17
    .quad isr_stub_0x18
    .quad isr_stub_0x19
    .quad isr_stub_0x1a
    .quad isr_stub_0x1b
    .quad isr_stub_0x1c
    .quad isr_stub_0x1d
    .quad isr_stub_0x1e
    .quad isr_stub_0x1f
    .quad isr_stub_0x20
    .quad isr_stub_0x21
    .quad isr_stub_0x22
    .quad isr_stub_0x23
    .quad isr_stub_0x24
    .quad isr_stub_0x25
    .quad isr_stub_0x26
    .quad isr_stub_0x27
    .quad isr_stub_0x28
    .quad isr_stub_0x29
    .quad isr_stub_0x2a
    .quad isr_stub_0x2b
    .quad isr_stub_0x2c
    .quad isr_stub_0x2d
    .quad isr_stub_0x2e
    .quad isr_stub_0x2f
    .quad isr_stub_0x30
    .quad isr_stub_0x31
    .quad isr_stub_0x32
    .quad isr_stub_0x33
    .quad isr_stub_0x34
    .quad isr_stub_0x35
    .quad isr_stub_0x36
    .quad isr_stub_0x37
    .quad isr_stub_0x38
    .quad isr_stub_0x39
    .quad isr_stub_0x3a
    .quad isr_stub_0x3b
    .quad isr_stub_0x3c
    .quad isr_stub_0x3d
    .quad isr_stub_0x3e
    .quad isr_stub_0x3f
    .quad isr_stub_0x40
    .quad isr_stub_0x41
    .quad isr_stub_0x42
    .quad isr_stub_0x43
    .quad isr_stub_0x44
    .quad isr_stub_0x45
    .quad isr_stub_0x46
    .quad isr_stub_0x47
    .quad isr_stub_0x48
    .quad isr_stub_0x49
    .quad isr_stub_0x4a
    .quad isr_stub_0x4b
    .quad isr_stub_0x4c
    .quad isr_stub_0x4d
    .quad isr_stub_0x4e
    .quad isr_stub_0x4f
    .quad isr_stub_0x50
    .quad isr_stub_0x51
    .quad isr_stub_0x52
    .quad isr_stub_0x53
    .quad isr_stub_0x54
    .quad isr_stub_0x55
    .quad isr_stub_0x56
    .quad isr_stub_0x57
    .quad isr_stub_0x58
    .quad isr_stub_0x59
    .quad isr_stub_0x5a
    .quad isr_stub_0x5b
    .quad isr_stub_0x5c
    .quad isr_stub_0x5d
    .quad isr_stub_0x5e
    .quad isr_stub_0x5f
    .quad isr_stub_0x60
    .quad isr_stub_0x61
    .quad isr_stub_0x62
    .quad isr_stub_0x63
    .quad isr_stub_0x64
    .quad isr_stub_0x65
    .quad isr_stub_0x66
    .quad isr_stub_0x67
    .quad isr_stub_0x68
    .quad isr_stub_0x69
    .quad isr_stub_0x6a
    .quad isr_stub_0x6b
    .quad isr_stub_0x6c
    .quad isr_stub_0x6d
    .quad isr_stub_0x6e
    .quad isr_stub_0x6f
    .quad isr_stub_0x70
    .quad isr_stub_0x71
    .quad isr_stub_0x72
    .quad isr_stub_0x73
    .quad isr_stub_0x74
    .quad isr_stub_0x75
    .quad isr_stub_0x76
    .quad isr_stub_0x77
    .quad isr_stub_0x78
    .quad isr_stub_0x79
    .quad isr_stub_0x7a
    .quad isr_stub_0x7b
    .quad isr_stub_0x7c
    .quad isr_stub_0x7d
    .quad isr_stub_0x7e
    .quad isr_stub_0x7f
    .quad isr_stub_0x80
    .quad isr_stub_0x81
    .quad isr_stub_0x82
    .quad isr_stub_0x83
    .quad isr_stub_0x84
    .quad isr_stub_0x85
    .quad isr_stub_0x86
    .quad isr_stub_0x87
    .quad isr_stub_0x88
    .quad isr_stub_0x89
    .quad isr_stub_0x8a
    .quad isr_stub_0x8b
    .quad isr_stub_0x8c
    .quad isr_stub_0x8d
    .quad isr_stub_0x8e
    .quad isr_stub_0x8f
    .quad isr_stub_0x90
    .quad isr_stub_0x91
    .quad isr_stub_0x92
    .quad isr_stub_0x93
    .quad isr_stub_0x94
    .quad isr_stub_0x95
    .quad isr_stub_0x96
    .quad isr_stub_0x97
    .quad isr_stub_0x98
    .quad isr_stub_0x99
    .quad isr_stub_0x9a
    .quad isr_stub_0x9b
    .quad isr_stub_0x9c
    .quad isr_stub_0x9d
    .quad isr_stub_0x9e
    .quad isr_stub_0x9f
    .quad isr_stub_0xa0
    .quad isr_stub_0xa1
    .quad isr_stub_0xa2
    .quad isr_stub_0xa3
    .quad isr_stub_0xa4
    .quad isr_stub_0xa5
    .quad isr_stub_0xa6
    .quad isr_stub_0xa7
    .quad isr_stub_0xa8
    .quad isr_stub_0xa9
    .quad isr_stub_0xaa
    .quad isr_stub_0xab
    .quad isr_stub_0xac
    .quad isr_stub_0xad
    .quad isr_stub_0xae
    .quad isr_stub_0xaf
    .quad isr_stub_0xb0
    .quad isr_stub_0xb1
    .quad isr_stub_0xb2
    .quad isr_stub_0xb3
    .quad isr_stub_0xb4
    .quad isr_stub_0xb5
    .quad isr_stub_0xb6
    .quad isr_stub_0xb7
    .quad isr_stub_0xb8
    .quad isr_stub_0xb9
    .quad isr_stub_0xba
    .quad isr_stub_0xbb
    .quad isr_stub_0xbc
    .quad isr_stub_0xbd
    .quad isr_stub_0xbe
    .quad isr_stub_0xbf
    .quad isr_stub_0xc0
    .quad isr_stub_0xc1
    .quad isr_stub_0xc2
    .quad isr_stub_0xc3
    .quad isr_stub_0xc4
    .quad isr_stub_0xc5
    .quad isr_stub_0xc6
    .quad isr_stub_0xc7
    .quad isr_stub_0xc8
    .quad isr_stub_0xc9
    .quad isr_stub_0xca
    .quad isr_stub_0xcb
    .quad isr_stub_0xcc
    .quad isr_stub_0xcd
    .quad isr_stub_0xce
    .quad isr_stub_0xcf
    .quad isr_stub_0xd0
    .quad isr_stub_0xd1
    .quad isr_stub_0xd2
    .quad isr_stub_0xd3
    .quad isr_stub_0xd4
    .quad isr_stub_0xd5
    .quad isr_stub_0xd6
    .quad isr_stub_0xd7
    .quad isr_stub_0xd8
    .quad isr_stub_0xd9
    .quad isr_stub_0xda
    .quad isr_stub_0xdb
    .quad isr_stub_0xdc
    .quad isr_stub_0xdd
    .quad isr_stub_0xde
    .quad isr_stub_0xdf
    .quad isr_stub_0xe0
    .quad isr_stub_0xe1
    .quad isr_stub_0xe2
    .quad isr_stub_0xe3
    .quad isr_stub_0xe4
    .quad isr_stub_0xe5
    .quad isr_stub_0xe6
    .quad isr_stub_0xe7
    .quad isr_stub_0xe8
    .quad isr_stub_0xe9
    .quad isr_stub_0xea
    .quad isr_stub_0xeb
    .quad isr_stub_0xec
    .quad isr_stub_0xed
    .quad isr_stub_0xee
    .quad isr_stub_0xef
    .quad isr_stub_0xf0
    .quad isr_stub_0xf1
    .quad isr_stub_0xf2
    .quad isr_stub_0xf3
    .quad isr_stub_0xf4
    .quad isr_stub_0xf5
    .quad isr_stub_0xf6
    .quad isr_stub_0xf7
    .quad isr_stub_0xf8
    .quad isr_stub_0xf9
    .quad isr_stub_0xfa
    .quad isr_stub_0xfb
    .quad isr_stub_0xfc
    .quad isr_stub_0xfd
    .quad isr_stub_0xfe
    .quad isr_stub_0xff
