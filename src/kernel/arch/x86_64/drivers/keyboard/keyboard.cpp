#include "keyboard.hpp"
#include "scancodes.hpp"

#include "arch/x86_64/drivers/apic/apic.hpp"
#include <arch/x86_64/interrupts/irq.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <kernel/log/log.hpp>

#include <cstdint>
#include <cstddef>

namespace x86_64::drivers::keyboard {
    constexpr std::size_t BUFFER_LEN = 16;
    
    static bool extended_pending = false;
    static bool shift_held = false;
    static bool control_held = false;
    static bool alt_held = false;
    static bool caps_lock = false;

    static KeyEvent event_buffer[BUFFER_LEN];

    static std::size_t buffer_read  = 0;
    static std::size_t buffer_write = 0;

    void handle_extended_key(std::uint8_t byte, bool released) {
        auto scancode = static_cast<ExtendedScanCode>(byte);

        if (scancode == ExtendedScanCode::RightCtrl) {
            control_held = !released;
        } else if (scancode == ExtendedScanCode::RightAlt) {
            alt_held = !released;
        }
        
        KeyEvent* event = &event_buffer[buffer_write];

        event->scancode = ScanCode::Nil;
        event->extended_scancode = scancode;
        event->released = released;
        event->shift_held = shift_held;
        event->control_held = control_held;
        event->alt_held = alt_held;
        event->caps_lock_on = caps_lock;
        
        if ((buffer_write + 1) % BUFFER_LEN == buffer_read) {
            buffer_read = (buffer_read + 1) % BUFFER_LEN;
        }

        buffer_write = (buffer_write + 1) % BUFFER_LEN;
    }

    void handle_standard_key(std::uint8_t byte, bool released) {
        auto scancode = static_cast<ScanCode>(byte);

        if (scancode == ScanCode::LeftShift || scancode == ScanCode::RightShift) {
            shift_held = !released;
        } else if (scancode == ScanCode::LeftCtrl) {
            control_held = !released;
        } else if (scancode == ScanCode::LeftAlt) {
            alt_held = !released;
        } else if (scancode == ScanCode::CapsLock && !released) {
            caps_lock = !caps_lock;
        }

        KeyEvent* event = &event_buffer[buffer_write];

        event->scancode = scancode;
        event->extended_scancode = ExtendedScanCode::Nil;
        event->released = released;
        event->shift_held = shift_held;
        event->control_held = control_held;
        event->alt_held = alt_held;
        event->caps_lock_on = caps_lock;

        if ((buffer_write + 1) % BUFFER_LEN == buffer_read) {
            buffer_read = (buffer_read + 1) % BUFFER_LEN;
        }

        buffer_write = (buffer_write + 1) % BUFFER_LEN;
    }

    KeyEvent* poll() {
        if (buffer_read == buffer_write) {
            return nullptr;
        }

        KeyEvent* event = &event_buffer[buffer_read];
        buffer_read = (buffer_read + 1) % BUFFER_LEN;

        return event;
    }

    KeyEvent* read() {
        while (true) {
            if (KeyEvent* event = poll()) {
                return event;
            }

            cpu::hlt();
        }
    }
    
    void keyboard_interrupt_handler(std::uint32_t vector) {
        std::uint8_t byte = cpu::inb(0x60);

        if (byte == EXTENDED_PREFIX) {
            extended_pending = true;
            apic::send_eoi();
            return;
        }

        bool released = (byte & RELEASE_MASK) > 0;
        std::uint8_t scancode = byte & ~RELEASE_MASK;

        if (extended_pending) {
            extended_pending = false;
            handle_extended_key(scancode, released);
        } else {
            handle_standard_key(scancode, released);
        }

        apic::send_eoi();
    }

    void disable_ps2_devices() {
        cpu::outb(0x64, 0xAD);
        cpu::outb(0x64, 0xA7);
    }

    void flush_output_buffer() {
        cpu::inb(0x60);
    }

    void configure() {
        cpu::outb(0x64, 0x20);

        while (true) {
            std::uint8_t status = cpu::inb(0x64);

            if (status & 1) {
                break;
            }
        }

        std::uint8_t config = cpu::inb(0x60);

        // Clear bits 0, 4, and 6
        config &= (0 << 0);
        config &= (0 << 4);
        config &= (0 << 6);

        cpu::outb(0x64, 0x60);

        while (true) {
            std::uint8_t status = cpu::inb(0x64);

            if ((status & 0b00000010) == 0) {
                break;
            }
        }

        cpu::outb(0x60, config);
    }
    
    void init() {
        kernel::log::init_start("Keyboard");

        //disable_ps2_devices();
        //flush_output_buffer();
        //configure();

        x86_64::irq::register_irq_handler(33, keyboard_interrupt_handler);

        kernel::log::init_end("Keyboard");
    }
}
