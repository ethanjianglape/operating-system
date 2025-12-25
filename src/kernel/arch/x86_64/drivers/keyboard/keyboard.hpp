#pragma once

#include "scancodes.hpp"

namespace x86_64::drivers::keyboard {
    struct KeyEvent {
        ScanCode scancode;
        ExtendedScanCode extended_scancode;
        bool released;
        bool shift_held;
        bool control_held;
        bool alt_held;
        bool caps_lock_on;
    };
    
    void init();

    KeyEvent* read();
    KeyEvent* poll();
}
