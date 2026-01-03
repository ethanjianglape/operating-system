/**
 * @file pit.cpp
 * @brief Programmable Interval Timer (8254 PIT) driver for APIC calibration.
 *
 * The PIT is a legacy timer chip dating back to the original IBM PC (1981).
 * It runs at exactly 1.193182 MHz - this frequency has been constant across
 * all PCs for 40+ years for backwards compatibility.
 *
 * We ONLY use the PIT for one purpose: calibrating the APIC timer. The APIC
 * timer runs at an unknown frequency (varies by CPU), so we use the PIT's
 * known frequency as a reference. After calibration, the PIT is never touched
 * again - the APIC timer handles all timing needs.
 *
 * See apic.cpp for how this calibration works.
 */

#include "pit.hpp"
#include <arch/x86_64/cpu/cpu.hpp>

namespace x86_64::drivers::pit {

    // =========================================================================
    // sleep_ms - Blocking delay using PIT one-shot mode
    // =========================================================================
    //
    // This function blocks (busy-waits) for approximately 'ms' milliseconds.
    // It's only used during early boot for APIC timer calibration.
    //
    // How it works:
    //   1. Calculate how many PIT ticks = ms milliseconds
    //   2. Program the PIT in Mode 0 (one-shot): counts down to 0, then stops
    //   3. Poll the PIT status until the "output" bit goes high (count = 0)
    //
    // Mode 0 (Interrupt on Terminal Count):
    //   - Load a count value
    //   - Counter decrements at 1.193182 MHz
    //   - When it hits 0, the OUT pin goes high
    //   - Counter stops (one-shot, not periodic)

    /**
     * @brief Busy-waits for approximately the specified number of milliseconds.
     *
     * Uses PIT channel 0 in one-shot mode. Only used during early boot for
     * APIC timer calibration — not suitable for general timing.
     *
     * @param ms Number of milliseconds to wait.
     */
    void sleep_ms(std::uint32_t ms) {
        // Build the command byte for the PIT:
        //   Bits 6-7: Channel select (00 = channel 0)
        //   Bits 4-5: Access mode (11 = lobyte/hibyte)
        //   Bits 1-3: Operating mode (000 = mode 0, one-shot)
        //   Bit 0:    BCD/Binary (0 = binary)
        const std::uint8_t command = CMD_CHANNEL_0 | CMD_ACCESS_LOHI | CMD_MODE_0 | CMD_BINARY;

        // Calculate the divisor: how many ticks for 'ms' milliseconds?
        // divisor = frequency × time = 1193182 Hz × (ms / 1000)
        const std::uint16_t divisor = (BASE_FREQUENCY * ms) / 1000;
        const std::uint8_t low = divisor & 0xFF;
        const std::uint8_t high = (divisor >> 8) & 0xFF;

        // Program the PIT: send command, then low byte, then high byte
        cpu::outb(COMMAND_REGISTER, command);
        cpu::outb(CHANNEL_0_DATA_PORT, low);
        cpu::outb(CHANNEL_0_DATA_PORT, high);

        // Poll until the countdown completes.
        // We use the "Read Back" command to check the status without affecting the count.
        //
        // Read Back command (0xE2):
        //   Bit 7-6: 11 = Read Back command
        //   Bit 5:   0 = Latch status (we want this)
        //   Bit 4:   1 = Don't latch count
        //   Bit 3-1: 001 = Select channel 0
        //   Bit 0:   0 = Reserved
        //
        // Status byte bit 7 (0x80) = OUT pin state
        //   When the countdown reaches 0, OUT goes high (bit 7 = 1)
        while (true) {
            cpu::outb(COMMAND_REGISTER, 0xE2);  // Read Back: status of channel 0
            const auto status = cpu::inb(CHANNEL_0_DATA_PORT);

            if (status & 0x80) {  // OUT pin high = countdown complete
                break;
            }
        }
    }
}
