/**
 * @file context.hpp
 * @brief ContextFrame structure for cooperative context switching.
 *
 * This struct matches the stack layout created by context_switch() when it
 * saves callee-saved registers. See context_switch.s for detailed explanation.
 *
 * For NEW processes, we manually construct a ContextFrame as a "launch packet":
 *
 *   ContextFrame* frame = ...;  // At top of new process's kernel stack
 *   frame->rip = userspace_entry_trampoline;  // Where 'ret' will jump
 *   frame->r15 = user_entry_point;            // Trampoline reads this
 *   frame->r14 = user_stack_pointer;          // Trampoline reads this
 *   frame->r13 = frame->r12 = frame->rbx = frame->rbp = 0;
 *
 * When context_switch() "restores" this fake frame, it pops our values into
 * registers and "returns" to the trampoline, which uses r15/r14 to build an
 * iretq frame and enter userspace. From context_switch()'s perspective, it's
 * just a normal context restore — it doesn't know it's launching a new process.
 */

#pragma once

#include <cstdint>

namespace x86_64::context {
    /**
     * Layout of saved registers on the kernel stack during context_switch().
     * Must match the push/pop order in context_switch.s exactly.
     */
    struct [[gnu::packed]] ContextFrame {
        std::uint64_t r15, r14, r13, r12;  // Callee-saved (used by trampoline)
        std::uint64_t rbx, rbp;            // Callee-saved
        std::uint64_t rip;                 // Return address (from 'call' insn)
    };
}
