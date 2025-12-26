#include "serial.hpp"

#include <containers/kstring.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <kernel/console/console.hpp>

namespace x86_64::drivers::serial {
    static bool is_transmit_ready() {
        return (cpu::inb(COM1 + LINE_STATUS) & LSR_TRANSMIT_EMPTY) != 0;
    }
    
    void init() {
        cpu::outb(COM1 + INT_ENABLE, 0x00);    // Disable interrupts
        cpu::outb(COM1 + LINE_CTRL, 0x80);     // Enable DLAB (set baud rate divisor)
        cpu::outb(COM1 + DATA, 0x03);          // Divisor low byte (38400 baud)
        cpu::outb(COM1 + INT_ENABLE, 0x00);    // Divisor high byte
        cpu::outb(COM1 + LINE_CTRL, 0x03);     // 8 bits, no parity, one stop bit (8N1)
        cpu::outb(COM1 + FIFO_CTRL, 0xC7);     // Enable FIFO, clear, 14-byte threshold
        cpu::outb(COM1 + MODEM_CTRL, 0x0B);    // IRQs enabled, RTS/DSR set
    }

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
