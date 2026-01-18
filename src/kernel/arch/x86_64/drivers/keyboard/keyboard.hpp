#pragma once

#include "ps2.hpp"
#include <cstdint>

namespace x86_64::drivers::keyboard {
    // =========================================================================
    // Key Event
    // =========================================================================
    //
    // Represents a keyboard event (press or release). Currently uses PS/2
    // scancodes directly - a future abstraction could use generic keycodes
    // that multiple backends (PS/2, USB HID) translate to.

    struct KeyEvent {
        ScanCode scancode;
        ExtendedScanCode extended_scancode;
        bool released;
        bool shift_held;
        bool control_held;
        bool alt_held;
        bool caps_lock_on;
    };

    // =========================================================================
    // Keyboard API
    // =========================================================================

    /**
     * @brief Initializes the keyboard subsystem and available backends.
     */
    void init();

    /**
     * @brief Polls for a key event without blocking.
     * @return Pointer to KeyEvent if available, nullptr otherwise.
     */
    KeyEvent* poll();

    /**
     * @brief Blocks until a key event is available.
     * @return Pointer to KeyEvent.
     */
    KeyEvent* read();

    /**
     * @brief Pushes a key event to the buffer (called by backends).
     * @param event The key event to push.
     */
    void push_event(const KeyEvent& event);

    /**
     * @brief Updates modifier state (called by backends before push_event).
     */
    void update_modifiers(ScanCode scancode, ExtendedScanCode extended, bool released);

    /**
     * @brief Gets current modifier state for building KeyEvent.
     */
    bool is_shift_held();
    bool is_control_held();
    bool is_alt_held();
    bool is_caps_lock_on();
}
