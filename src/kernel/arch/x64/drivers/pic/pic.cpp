/**
 * @file pic.cpp
 * @brief Legacy 8259 PIC driver — disables the PIC in favor of the APIC.
 *
 * What is the PIC?
 *
 *   The 8259A PIC is legacy interrupt controller hardware dating back to the
 *   original IBM PC (1981). It routes hardware interrupts from devices (keyboard,
 *   timer, disk, etc.) to the CPU. The original PC had one PIC with 8 IRQ lines;
 *   the AT added a second "slave" PIC cascaded through IRQ 2, giving 15 usable IRQs.
 *
 *   ┌─────────────┐         ┌─────────────┐
 *   │   Master    │         │    Slave    │
 *   │  PIC (8259) │         │  PIC (8259) │
 *   ├─────────────┤         ├─────────────┤
 *   │ IRQ 0: Timer│         │ IRQ 8: RTC  │
 *   │ IRQ 1: Keybd│         │ IRQ 9: ACPI │
 *   │ IRQ 2: ─────┼────────►│ IRQ 10      │
 *   │ IRQ 3: COM2 │         │ IRQ 11      │
 *   │ IRQ 4: COM1 │         │ IRQ 12:Mouse│
 *   │ IRQ 5: LPT2 │         │ IRQ 13: FPU │
 *   │ IRQ 6:Floppy│         │ IRQ 14: ATA │
 *   │ IRQ 7: LPT1 │         │ IRQ 15: ATA │
 *   └──────┬──────┘         └─────────────┘
 *          │
 *          ▼
 *         CPU
 *
 * Why Do We Disable It?
 *
 *   Modern x86-64 systems use the APIC (Advanced Programmable Interrupt Controller)
 *   instead. The APIC is far more capable:
 *     - Supports more than 15 IRQs (up to 224 interrupt vectors)
 *     - Required for multi-core/multi-processor systems
 *     - Supports MSI (Message Signaled Interrupts) for PCIe devices
 *     - Better priority handling and interrupt routing
 *
 *   However, the legacy PIC still exists for backwards compatibility and can
 *   generate spurious interrupts if not properly disabled. We disable it by
 *   masking all IRQ lines (writing 0xFF to both data ports), which tells the
 *   PIC to ignore all interrupt requests.
 *
 *   After this, we use the Local APIC and I/O APIC exclusively for interrupts.
 *
 * The Ports:
 *
 *   PIC1 (Master): Command = 0x20, Data = 0x21
 *   PIC2 (Slave):  Command = 0xA0, Data = 0xA1
 *
 *   Writing 0xFF to the data port sets the Interrupt Mask Register (IMR) to
 *   mask (disable) all 8 IRQ lines on that PIC.
 */

#include "pic.hpp"

#include <arch/x64/cpu/cpu.hpp>
#include <kassert/kassert.hpp>
#include <log/log.hpp>

namespace x64::drivers::pic {
/**
 * @brief Disables the legacy PIC by masking all IRQ lines.
 *
 * Writes 0xFF to both PIC data ports to mask all interrupts, then verifies
 * the masks were applied. This must be called before enabling the APIC.
 *
 * @return true if successfully disabled, false otherwise (also panics on failure).
 */
bool init()
{
    // Mask all IRQs on both PICs (0xFF = all 8 bits set = all IRQs masked)
    cpu::outb(PIC1_DATA, 0xFF);
    cpu::outb(PIC2_DATA, 0xFF);

    // Verify the masks were set correctly
    const std::uint8_t master = cpu::inb(PIC1_DATA);
    const std::uint8_t slave = cpu::inb(PIC2_DATA);
    const bool success = master == 0xFF && slave == 0xFF;

    kassert(success, "Failed to disable PIC");

    log::info("PIC: disabled");

    return success;
}

}
