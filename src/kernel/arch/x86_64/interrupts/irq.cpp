/**
 * @file irq.cpp
 * @brief IRQ dispatcher and exception handling for interrupts.
 *
 * This is the C++ side of interrupt handling - where we actually DO something
 * in response to interrupts. The assembly stubs (isr.s) just save registers
 * and call us; we decide what action to take.
 *
 * Complete Interrupt Flow:
 *
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │  1. Interrupt occurs (hardware IRQ, CPU exception, or INT instruction)  │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *                                      │
 *                                      ▼
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │  2. CPU looks up IDT[vector] to find handler address                    │
 *   │     (IDT was set up by idt.cpp during boot)                             │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *                                      │
 *                                      ▼
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │  3. CPU pushes SS, RSP, RFLAGS, CS, RIP (+ error code for some)         │
 *   │     and jumps to the handler (one of our 256 stubs in isr.s)            │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *                                      │
 *                                      ▼
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │  4. ISR stub (isr.s) pushes vector number and dummy error code (if      │
 *   │     needed), then jumps to isr_common                                   │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *                                      │
 *                                      ▼
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │  5. isr_common saves all general-purpose registers, then calls          │
 *   │     interrupt_handler(frame) - that's THIS file!                        │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *                                      │
 *                                      ▼
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │  6. interrupt_handler dispatches based on vector number:                │
 *   │       - Vectors 0-31:  CPU exceptions → handle_exception() → panic      │
 *   │       - Vectors 32+:   Hardware IRQs  → handle_irq() → registered fn    │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *                                      │
 *                                      ▼
 *   ┌─────────────────────────────────────────────────────────────────────────┐
 *   │  7. After handler returns, isr_common restores registers and executes   │
 *   │     IRETQ to resume the interrupted code                                │
 *   └─────────────────────────────────────────────────────────────────────────┘
 *
 * Why One Dispatcher?
 *
 *   We have 256 separate ISR stubs because the CPU doesn't tell us which
 *   interrupt occurred - we have to know based on which stub was called.
 *   Each stub pushes its own vector number onto the stack.
 *
 *   But once we know the vector number, there's no reason to have 256 separate
 *   C++ functions. Instead, all stubs call the same interrupt_handler(), which
 *   reads the vector from the stack frame and dispatches accordingly.
 *
 * Handler Registration:
 *
 *   For hardware IRQs (vectors 32-255), drivers can register callbacks:
 *
 *     irq::register_irq_handler(0x20, timer_handler);  // Timer on IRQ 0
 *     irq::register_irq_handler(0x21, keyboard_handler);  // Keyboard on IRQ 1
 *
 *   When that vector fires, we call the registered function. If no handler
 *   is registered, the interrupt is silently ignored.
 *
 * Exception Handling:
 *
 *   CPU exceptions (vectors 0-31) indicate serious errors like divide-by-zero
 *   or page faults. Currently we just panic - a real OS would handle some of
 *   these more gracefully (e.g., page faults for demand paging).
 */

#include "irq.hpp"

#include <arch/x86_64/cpu/cpu.hpp>
#include <log/log.hpp>
#include <cstdint>

namespace x86_64::irq {
    // Handler function pointers for each interrupt vector.
    // Vectors 0-31 are exceptions (handled specially), 32-255 are IRQs.
    static irq_handler_fn irq_handlers[NUM_IRQ_HANDLERS] = {nullptr};

    /**
     * @brief Registers a callback function for a hardware IRQ vector.
     * @param vector Interrupt vector number (must be > 31 for IRQs).
     * @param handler Function to call when this interrupt fires.
     */
    void register_irq_handler(const std::uint32_t vector, irq_handler_fn handler) {
        if (vector > EXC_MAX && vector < NUM_IRQ_HANDLERS) {
            irq_handlers[vector] = handler;
        }
    }

    // CPU exception names for debugging output
    constexpr const char* const exception_names[] = {
        "Divide Error (#DE)",
        "Debug (#DB)",
        "Non-Maskable Interrupt (NMI)",
        "Breakpoint (#BP)",
        "Overflow (#OF)",
        "Bound Range Exceeded (#BR)",
        "Invalid Opcode (#UD)",
        "Device Not Available (#NM)",
        "Double Fault (#DF)",
        "Coprocessor Segment Overrun",
        "Invalid TSS (#TS)",
        "Segment Not Present (#NP)",
        "Stack Segment Fault (#SS)",
        "General Protection Fault (#GP)",
        "Page Fault (#PF)",
        "Reserved",
        "x87 FPU Error (#MF)",
        "Alignment Check (#AC)",
        "Machine Check (#MC)",
        "SIMD Floating-Point (#XM)",
        "Virtualization Exception (#VE)",
        "Control Protection (#CP)",
        "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
        "Hypervisor Injection (#HV)",
        "VMM Communication (#VC)",
        "Security Exception (#SX)",
        "Reserved"
    };

    /**
     * @brief Decodes and logs page fault details from CR2 and error code.
     * @param error The page fault error code pushed by the CPU.
     */
    static void handle_page_fault(std::uint64_t error) {
        std::uint64_t fault_addr;
        asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
        log::error("Faulting Address: ", fmt::hex{fault_addr});

        // Decode the page fault error code
        log::error("Cause: ",
                   (error & 0x1) ? "Protection violation" : "Page not present",
                   (error & 0x2) ? ", Write" : ", Read",
                   (error & 0x4) ? ", User mode" : ", Kernel mode",
                   (error & 0x8) ? ", Reserved bit set" : "",
                   (error & 0x10) ? ", Instruction fetch" : "");
    }

    // =========================================================================
    // Exception Handler
    // =========================================================================
    //
    // For simplicity, our kernel panics on ALL CPU exceptions. This is fine for
    // learning, but a real OS like Linux handles many exceptions more gracefully:
    //
    //   - Divide by zero (#DE): Linux sends SIGFPE to the process, not a kernel panic.
    //     A C program with "int a = 1/0;" crashes that process, not the whole OS.
    //
    //   - Page fault (#PF): Linux uses this for demand paging, copy-on-write, and
    //     swapping. Only truly invalid accesses become SIGSEGV to the process.
    //
    //   - Invalid opcode (#UD): Could be used to emulate missing CPU instructions,
    //     or sends SIGILL to the process.
    //
    //   - Breakpoint (#BP): Debuggers use INT3 to set breakpoints; the kernel
    //     notifies the debugger via ptrace, not a panic.
    //
    // The key insight: exceptions in USERSPACE should kill the process (or be
    // handled), not the kernel. Only exceptions in KERNEL MODE are truly fatal.
    // We don't distinguish yet because we're still building the basics.

    [[noreturn]]
    static void handle_exception(const InterruptFrame* frame) {
        const auto vector = frame->vector;
        const auto error = frame->err;

        log::error("===== !!KERNEL PANIC!! =====");
        log::error("CPU Exception: ", exception_names[vector]);
        log::error("Vector: ", vector, " (", fmt::hex{vector}, ")");
        log::error("Error Code: ", error, " (", fmt::hex{error}, ")");

        if (vector == EXC_PAGE_FAULT) {
            handle_page_fault(error);
        }

        // Dump full CPU state for debugging
        cpu::dump(frame);

        log::error("========================");

        cpu::cli();

        while (true) {
            cpu::hlt();
        }
    }

    static void handle_irq(InterruptFrame* frame) {
        const auto vector = frame->vector;
        
        if (irq_handlers[vector]) {
            irq_handlers[vector](frame);
        } else {
            log::debug("Unhandled IRQ: vector ", vector, " (", fmt::hex{vector} ,")");
        }
    }
}

// =============================================================================
// Main Entry Point (called from isr.s)
// =============================================================================

/**
 * @brief Main interrupt dispatcher called from assembly ISR stubs.
 *
 * This is extern "C" so the assembly can call it by name without C++ mangling.
 * Dispatches to handle_exception() for vectors 0-31, or handle_irq() for 32+.
 *
 * @param frame Pointer to the saved register state and vector/error code.
 */
extern "C"
void interrupt_handler(x86_64::irq::InterruptFrame* frame) {
    const auto vector = frame->vector;

    if (vector <= x86_64::irq::EXC_MAX) {
        x86_64::irq::handle_exception(frame);
    } else {
        x86_64::irq::handle_irq(frame);
    }
}
