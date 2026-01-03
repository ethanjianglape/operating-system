/**
 * @file serial.cpp
 * @brief Serial port (UART) driver for COM1 debug output.
 *
 * The 8250/16550 UART is a legacy serial controller present on all PCs.
 * We use COM1 (0x3F8) as a simple debug output channel - QEMU and other
 * emulators can redirect this to the terminal, making it invaluable for
 * early boot debugging before more complex output is available.
 *
 * UART Register Layout (relative to base port 0x3F8):
 *
 *   Offset  DLAB=0            DLAB=1
 *   ------  ----------------  ------------------
 *   +0      Data (RX/TX)      Divisor Latch Low
 *   +1      Interrupt Enable  Divisor Latch High
 *   +2      FIFO Control      FIFO Control
 *   +3      Line Control      Line Control
 *   +4      Modem Control     Modem Control
 *   +5      Line Status       Line Status
 *   +6      Modem Status      Modem Status
 *   +7      Scratch           Scratch
 *
 * The DLAB (Divisor Latch Access Bit) in the Line Control register changes
 * what offsets +0 and +1 access. When DLAB=1, we can set the baud rate
 * divisor. When DLAB=0, offset +0 is the data register for TX/RX.
 *
 * Baud rate = 115200 / divisor. With divisor=3, we get 38400 baud.
 */

#include "serial.hpp"

#include <containers/kstring.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <console/console.hpp>

namespace x86_64::drivers::serial {
    /**
     * @brief Checks if the UART transmit buffer is empty and ready for data.
     * @return true if ready to transmit, false if busy.
     */
    static bool is_transmit_ready() {
        return (cpu::inb(COM1 + LINE_STATUS) & LSR_TRANSMIT_EMPTY) != 0;
    }

    /**
     * @brief Initializes COM1 serial port to 38400 baud, 8N1 configuration.
     *
     * The initialization sequence:
     *   1. Disable interrupts (we poll instead)
     *   2. Enable DLAB to set baud rate divisor
     *   3. Set divisor to 3 (115200/3 = 38400 baud)
     *   4. Disable DLAB, set 8 data bits, no parity, 1 stop bit
     *   5. Enable and clear FIFOs
     *   6. Set modem control lines
     */
    void init() {
        cpu::outb(COM1 + INT_ENABLE, 0x00); // Disable interrupts
        cpu::outb(COM1 + LINE_CTRL, 0x80);  // Enable DLAB (set baud rate divisor)
        cpu::outb(COM1 + DATA, 0x03);       // Divisor low byte (38400 baud)
        cpu::outb(COM1 + INT_ENABLE, 0x00); // Divisor high byte
        cpu::outb(COM1 + LINE_CTRL, 0x03);  // 8 bits, no parity, one stop bit (8N1)
        cpu::outb(COM1 + FIFO_CTRL, 0xC7);  // Enable FIFO, clear, 14-byte threshold
        cpu::outb(COM1 + MODEM_CTRL, 0x0B); // IRQs enabled, RTS/DSR set
    }

    /**
     * @brief Writes a single character to the serial port.
     *
     * Blocks until the transmit buffer is ready. Converts LF to CRLF
     * for proper terminal line endings.
     *
     * @param c Character to write.
     * @return Always returns 1.
     */
    int putchar(char c) {
        while (!is_transmit_ready());

        if (c == '\n') {
            cpu::outb(COM1 + DATA, '\r');
        }

        cpu::outb(COM1 + DATA, c);

        return 1;
    }

    int puts(const kstring& str) {
        for (char c : str) {
            putchar(c);
        }

        return str.size();
    }

    int puts(const char* str) {
        int written = 0;
        
        while (*str) {
            written += putchar(*str++);
        }

        return written;
    }

    int puts(const unsigned char* str) {
        int written = 0;
        
        while (*str) {
            written += putchar(*str++);
        }

        return written;
    }
}
