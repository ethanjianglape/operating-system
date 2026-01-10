# OS Development Insights & Debugging Notes

Hard-won lessons from developing this kernel. These are the things that took hours to debug and aren't well explained elsewhere.

---

## x86_64 Syscall/Sysret

### Why `swapgs` can't be used unconditionally in interrupts

The `swapgs` instruction swaps the GS base MSR with the kernel GS base MSR. It's used to access per-CPU data in the kernel.

**The problem**: Interrupts can fire in both userspace AND kernel mode. If you're already in kernel mode (GS base is already the kernel value), calling `swapgs` will swap it to the *user* value - exactly backwards.

**Solution**: Check CS on the stack to determine where we came from:
```asm
isr_common:
    cmp $0x08, 24(%rsp)    # CS at offset 24 on entry
    je skip_swapgs
    swapgs
skip_swapgs:
    # ... handle interrupt ...

    cmp $0x08, 8(%rsp)     # CS at offset 8 after popping registers
    je skip_swapgs_exit
    swapgs
skip_swapgs_exit:
    iretq
```

**Why syscall is different**: The `syscall` instruction is ONLY executed from userspace. You will never receive a syscall from kernel mode. Therefore, `syscall_entry` can use unconditional `swapgs`.

---

### Why `extern constexpr` doesn't work in assembly

You might try to share constants between C++ and assembly:
```cpp
// C++
extern constexpr std::size_t PER_CPU_OFFSET = 16;
```
```asm
// Assembly - THIS DOESN'T WORK
mov %rsp, %gs:PER_CPU_OFFSET
```

**The problem**: `extern` makes the symbol externally visible, but the *address* of the constant is exported, not its value. In assembly, `PER_CPU_OFFSET` resolves to something like `0xFFFFFFFF80018518` (the address where 16 is stored), not `16`.

**Solution**: Define constants directly in assembly:
```asm
.set PER_CPU_SELF_OFFSET, 0
.set PER_CPU_KERNEL_RSP_OFFSET, 8
.set PER_CPU_USER_RSP_OFFSET, 16
```

Or use a header that both C++ and assembly include (with appropriate preprocessor guards).

---

### Why syscall_entry must preserve RAX for return value

After `syscall_dispatcher` returns, RAX contains the syscall return value. The entry code pushes all registers including RAX at the start. At the end:

```asm
# WRONG - overwrites return value with saved (old) rax
pop %rax

# CORRECT - skip saved rax, keep return value in rax
add $8, %rsp
```

---

## Segment Selectors and the GDT

### Why SS=0x18 instead of SS=0x1B causes #GP

Segment selectors have this format: `[Index:13][TI:1][RPL:2]`

- Index: Which GDT entry
- TI: Table Indicator (0=GDT, 1=LDT)
- RPL: Requested Privilege Level (0-3)

Your GDT:
- 0x08 = kernel code (index 1, RPL 0)
- 0x10 = kernel data (index 2, RPL 0)
- 0x18 = user data (index 3, RPL 0) ← Wrong for userspace!
- 0x1B = user data (index 3, RPL 3) ← Correct
- 0x23 = user code (index 4, RPL 3)

When returning to userspace (Ring 3), the CPU checks that RPL matches. Using 0x18 (RPL=0) for a Ring 3 process causes #GP.

**The fix**: Always OR with 3 for userspace segments:
```cpp
p->cs = 0x20 | 3;  // 0x23 - user code with RPL 3
p->ss = 0x18 | 3;  // 0x1B - user data with RPL 3
```

---

### Why saving frame->ss in the scheduler corrupts process state

The scheduler runs on timer interrupt. It saves the interrupt frame to the process struct:
```cpp
current->ss = frame->ss;  // BUG!
```

**The problem**: If the interrupt fires while in kernel mode (handling a syscall), `frame->ss` contains the *kernel* SS (0x10), not the user SS (0x1B). Saving this corrupts the process state.

**The insight**: User CS/SS are constants for a userspace process. They never change. Don't save them from the interrupt frame - just set them once at process creation.

---

## Scheduler and Blocking I/O

### Why the scheduler must check CS before context switching

When `yield_blocked()` does `sti; hlt`, timer interrupts still fire. The scheduler runs, sees a current process, and tries to save its state:

```cpp
void schedule(InterruptFrame* frame) {
    if (current != nullptr) {
        current->rip = frame->rip;  // Saves KERNEL rip!
        // ...
        frame->cs = p->cs;  // Sets to 0x23 (user)
        // iretq with kernel RIP + user CS = #GP
    }
}
```

**The problem**: The interrupt frame contains kernel addresses (we were in `yield_blocked`), but the scheduler tries to return with user segment selectors.

**Solution**: Don't context switch if we're in kernel mode handling a syscall:
```cpp
void schedule(InterruptFrame* frame) {
    if (current != nullptr && frame->cs == 0x08) {
        return;  // In kernel handling syscall, don't interfere
    }
    // ... normal scheduling ...
}
```

---

## Interrupt vs Syscall Entry

### The stack is completely different

**Interrupt entry** (CPU pushes automatically):
```
+40  SS
+32  RSP
+24  RFLAGS
+16  CS
+8   RIP
+0   Error code (or 0)
```

**Syscall entry** (CPU pushes NOTHING):
```
RCX = return RIP
R11 = return RFLAGS
RSP = still pointing to USER stack!
```

This is why syscall_entry must immediately switch stacks:
```asm
syscall_entry:
    swapgs
    mov %rsp, %gs:PER_CPU_USER_RSP_OFFSET  # Save user RSP
    mov %gs:PER_CPU_KERNEL_RSP_OFFSET, %rsp # Load kernel RSP
```

Trying to push registers before switching stacks would push onto the user stack - which might not even be mapped in kernel page tables.

---

## Debugging Tips

### #GP with error code = segment selector
The error code often IS the problematic segment selector. Error code 0x18 means something is wrong with segment 0x18.

### Check if problem is syscall or interrupt related
Remove syscalls from userspace program. If it works, problem is in syscall path. If it still crashes, problem is in interrupt/scheduler path.

### Log CS/SS values
When debugging context switches, log the segment values at every transition. They should be:
- Kernel: CS=0x08, SS=0x10
- User: CS=0x23, SS=0x1B

Anything else indicates corruption.

---

*Add new insights as you discover them. Future you (and other developers) will thank you.*
