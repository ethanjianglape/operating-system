#include "keyboard.hpp"

#include <arch/x86_64/cpu/cpu.hpp>
#include <containers/kvector.hpp>
#include <log/log.hpp>

namespace x86_64::drivers::keyboard {
    static kvector<KeyEvent> event_buffer;

    static bool shift_held = false;
    static bool control_held = false;
    static bool alt_held = false;
    static bool caps_lock = false;

    void update_modifiers(ScanCode scancode, ExtendedScanCode extended, bool released) {
        // Standard scancodes
        if (scancode == ScanCode::LeftShift || scancode == ScanCode::RightShift) {
            shift_held = !released;
        } else if (scancode == ScanCode::LeftCtrl) {
            control_held = !released;
        } else if (scancode == ScanCode::LeftAlt) {
            alt_held = !released;
        } else if (scancode == ScanCode::CapsLock && !released) {
            caps_lock = !caps_lock;
        }

        // Extended scancodes
        if (extended == ExtendedScanCode::RightCtrl) {
            control_held = !released;
        } else if (extended == ExtendedScanCode::RightAlt) {
            alt_held = !released;
        }
    }

    bool is_shift_held() { return shift_held; }
    bool is_control_held() { return control_held; }
    bool is_alt_held() { return alt_held; }
    bool is_caps_lock_on() { return caps_lock; }

    void push_event(const KeyEvent& event) {
        event_buffer.push_back(event);
    }

    KeyEvent* poll() {
        if (event_buffer.empty()) {
            return nullptr;
        }

        // Copy front element to static storage, then remove from buffer
        static KeyEvent current_event;
        current_event = event_buffer[0];
        event_buffer.erase(0);

        return &current_event;
    }

    KeyEvent* read() {
        while (true) {
            if (KeyEvent* event = poll()) {
                return event;
            }

            cpu::hlt();
        }
    }
    
    void init() {
        log::init_start("Keyboard");

        if (!ps2::init()) {
            log::warn("PS/2 keyboard initialization failed");
        }

        log::init_end("Keyboard");
    }
}
