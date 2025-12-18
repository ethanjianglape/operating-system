#include "serial.hpp"

#include <arch/x86_64/cpu/cpu.hpp>

#include <kernel/console/console.hpp>

namespace x86_64::drivers::serial {
    static kernel::console::ConsoleDriver console_driver = {
        .putchar = putchar,
        .mode = kernel::console::ConsoleMode::SERIAL
    };

    kernel::console::ConsoleDriver* get_console_driver() {
        return &console_driver;
    }

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

        puts("[INFO] MyOS booted into kernel_main()\n");
        puts("[INFO] Serial logging initialized\n");
        puts("[INFO] Starting kernel init process...\n");
    }

    void putchar(char c) {
        while (!is_transmit_ready());

        if (c == '\n') {
            cpu::outb(COM1 + DATA, '\r');
        }

        cpu::outb(COM1 + DATA, c);
    }

    void puts(const char* str) {
        while (*str) {
            putchar(*str++);
        }
    }
}
