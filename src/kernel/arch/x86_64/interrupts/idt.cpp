/**
 * @file idt.cpp
 * @brief Interrupt Descriptor Table (IDT) initialization for 64-bit long mode.
 *
 * What is the IDT?
 *   The IDT tells the CPU what code to run when an interrupt occurs. An
 *   "interrupt" is any event that diverts the CPU from normal execution:
 *
 *     - Exceptions (vectors 0-31): CPU-generated errors like divide-by-zero,
 *       page faults, general protection faults, etc.
 *     - Hardware IRQs (vectors 32+): External devices signaling the CPU
 *       (timer tick, keyboard press, disk I/O complete, etc.)
 *     - Software interrupts: Triggered by the INT instruction (e.g., int 0x80
 *       for syscalls)
 *
 *   Each interrupt has a "vector number" (0-255). The IDT is an array of 256
 *   entries, one per vector, each pointing to a handler function (ISR).
 *
 * IDT Entry Layout (16 bytes):
 *
 *   Unlike GDT entries (8 bytes), IDT entries in 64-bit mode are 16 bytes
 *   because they need a full 64-bit handler address.
 *
 *   127                              96 95                              64
 *   ┌──────────────────────────────────┬──────────────────────────────────┐
 *   │            Reserved              │      Offset [63:32]              │
 *   │          (must be 0)             │      (handler addr high)         │
 *   └──────────────────────────────────┴──────────────────────────────────┘
 *   63              48 47    40 39  35 34  32 31              16 15       0
 *   ┌─────────────────┬────────┬──────┬─────┬──────────────────┬──────────┐
 *   │  Offset [31:16] │  Attr  │  0   │ IST │    Selector      │Offset    │
 *   │  (handler mid)  │        │      │     │   (GDT entry)    │[15:0]    │
 *   └─────────────────┴────────┴──────┴─────┴──────────────────┴──────────┘
 *
 *   Attributes byte:
 *   ┌───┬───────┬───┬───────────┐
 *   │ P │  DPL  │ 0 │   Type    │
 *   │   │ (2b)  │   │   (4b)    │
 *   └───┴───────┴───┴───────────┘
 *     7   6   5   4   3       0
 *
 *     P    = Present (must be 1)
 *     DPL  = Descriptor Privilege Level - minimum ring that can trigger this
 *            interrupt via INT instruction (hardware interrupts ignore this)
 *     Type = 0xE for 64-bit interrupt gate (clears IF, disabling interrupts)
 *            0xF for 64-bit trap gate (leaves IF unchanged)
 *
 *   Common attribute values:
 *     0x8E = Present, DPL=0, Interrupt Gate  (kernel only)
 *     0xEE = Present, DPL=3, Interrupt Gate  (userspace can trigger via INT)
 *
 * Interrupt Vector Map:
 *
 *   ┌────────────┬──────────────────────────────────────────────────────────┐
 *   │  Vector    │  Description                                             │
 *   ├────────────┼──────────────────────────────────────────────────────────┤
 *   │  0x00      │  #DE - Divide Error                                      │
 *   │  0x01      │  #DB - Debug Exception                                   │
 *   │  0x02      │  NMI - Non-Maskable Interrupt                            │
 *   │  0x03      │  #BP - Breakpoint (INT3)                                 │
 *   │  0x04      │  #OF - Overflow                                          │
 *   │  0x05      │  #BR - Bound Range Exceeded                              │
 *   │  0x06      │  #UD - Invalid Opcode                                    │
 *   │  0x07      │  #NM - Device Not Available                              │
 *   │  0x08      │  #DF - Double Fault (has error code)                     │
 *   │  0x09      │  Coprocessor Segment Overrun (reserved)                  │
 *   │  0x0A      │  #TS - Invalid TSS (has error code)                      │
 *   │  0x0B      │  #NP - Segment Not Present (has error code)              │
 *   │  0x0C      │  #SS - Stack Segment Fault (has error code)              │
 *   │  0x0D      │  #GP - General Protection Fault (has error code)         │
 *   │  0x0E      │  #PF - Page Fault (has error code)                       │
 *   │  0x0F      │  Reserved                                                │
 *   │  0x10      │  #MF - x87 FPU Error                                     │
 *   │  0x11      │  #AC - Alignment Check (has error code)                  │
 *   │  0x12      │  #MC - Machine Check                                     │
 *   │  0x13      │  #XM - SIMD Floating-Point Exception                     │
 *   │  0x14      │  #VE - Virtualization Exception                          │
 *   │  0x15-0x1D │  Reserved                                                │
 *   │  0x1E      │  #SX - Security Exception (has error code)               │
 *   │  0x1F      │  Reserved                                                │
 *   ├────────────┼──────────────────────────────────────────────────────────┤
 *   │  0x20-0x2F │  Hardware IRQs (remapped PIC or APIC)                    │
 *   │  0x30-0x7F │  Available for other hardware/software                   │
 *   │  0x80      │  Syscall (legacy Linux convention, DPL=3)                │
 *   │  0x81-0xFF │  Available for other uses                                │
 *   └────────────┴──────────────────────────────────────────────────────────┘
 *
 * How Does This Actually Work?
 *
 *   Like the GDT, this code just fills out data structures in memory. The
 *   CPU instruction LIDT tells the CPU where to find our IDT:
 *
 *     lidt [idtr]  - Load IDT Register: tells CPU where our IDT lives
 *
 *   After LIDT, whenever an interrupt occurs, the CPU:
 *     1. Looks up IDT[vector] to find the handler address
 *     2. Pushes SS, RSP, RFLAGS, CS, RIP onto the stack (and error code if any)
 *     3. If changing privilege levels, switches to the kernel stack (from TSS)
 *     4. Jumps to the handler address
 *
 *   The handler (ISR) does its work, then executes IRETQ to return. IRETQ
 *   pops the saved state and resumes the interrupted code.
 *
 *   This is a one-time setup during boot. Once LIDT is called, the IDT stays
 *   fixed. The CPU automatically uses it for all interrupts without any
 *   further kernel involvement.
 *
 *   Important: The IDT itself doesn't contain interrupt handling code - it's
 *   just a lookup table of pointers. The actual handlers live in:
 *     - isr.s: Assembly stubs that save registers and call into C++
 *     - irq.cpp: C++ code that dispatches to exception handlers or IRQ handlers
 */

#include "idt.hpp"

#include <log/log.hpp>
#include <cstdint>

extern "C" void* isr_stub_table[];

namespace x86_64::idt {
    alignas(16) static IdtEntry idt_entries[IDT_MAX_DESCRIPTORS];
    alignas(16) static Idtr idtr;

    /**
     * @brief Configures an IDT entry for a specific interrupt vector.
     * @param vector Interrupt vector number (0-255).
     * @param isr_ptr Pointer to the interrupt service routine.
     * @param ist Interrupt Stack Table index (0 = don't switch stacks, 1-7 = use IST entry).
     * @param flags Attributes byte (IDT_KERNEL_INT or IDT_USER_INT).
     */
    void set_descriptor(std::uint8_t vector, void* isr_ptr, std::uint8_t ist, std::uint8_t flags) {
        IdtEntry* desc = &idt_entries[vector];

        const auto isr = reinterpret_cast<std::uintptr_t>(isr_ptr);

        // isr is the address of the actual code this IDT will execute when fired,
        // it is split up into 3 chunks in the IDT entry:
        desc->offset_low = isr & 0xFFFF;
        desc->offset_mid = (isr >> 16) & 0xFFFF;
        desc->offset_high = (isr >> 32) & 0xFFFFFFFF;
        
        desc->ist = ist & 0x7;
        desc->selector = KERNEL_CODE_SEL;
        desc->attributes = flags;
        desc->reserved = 0x0;
    }

    /**
     * @brief Initializes all 256 IDT entries and loads the IDT into the CPU.
     *
     * All vectors default to kernel-only (DPL=0) except vector 0x80 (syscall)
     * which is set to DPL=3 so userspace can trigger it via INT 0x80.
     */
    void init() {
        log::init_start("IDT");

        // idtr is just a "pointer" to our entire list of 256 entries, we use this
        // when calling the LIDT instruction so the CPU knows where to find them
        idtr.base = reinterpret_cast<std::uint64_t>(&idt_entries[0]);
        idtr.limit = sizeof(IdtEntry) * IDT_MAX_DESCRIPTORS - 1;

        for (std::size_t vector = 0; vector < IDT_MAX_DESCRIPTORS; vector++) {
            // By default, all interrupt vectors will only be callable by kernel code
            std::uint8_t flags = IDT_KERNEL_INT;
            std::uint8_t ist = 0x0;

            // However, int 0x80 (syscall) needs to be callable from userspace
            if (vector == IDT_VECTOR_SYSCALL) {
                flags = IDT_USER_INT;
            }

            set_descriptor(vector, isr_stub_table[vector], ist, flags);
        }

        // LIDT called a single time to tell the CPU where our IDT is located,
        // this is never called again during the lifecycle of the OS
        asm volatile("lidt %0" : : "m"(idtr));

        log::init_end("IDT");
    }
}

