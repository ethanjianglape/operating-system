#pragma once

#include <cstdint>
namespace x86_64::drivers::keyboard {

    // PS/2 Scancode Set 1 (make codes)
    // Key release = make code | 0x80
    // Extended keys are prefixed with 0xE0
    enum class ScanCode : std::uint8_t {
        Nil         = 0x00,
        
        // Row 1: Escape and Function keys
        Escape      = 0x01,
        F1          = 0x3B,
        F2          = 0x3C,
        F3          = 0x3D,
        F4          = 0x3E,
        F5          = 0x3F,
        F6          = 0x40,
        F7          = 0x41,
        F8          = 0x42,
        F9          = 0x43,
        F10         = 0x44,
        F11         = 0x57,
        F12         = 0x58,

        // Row 2: Number row
        Grave       = 0x29,  // ` ~
        Key1        = 0x02,
        Key2        = 0x03,
        Key3        = 0x04,
        Key4        = 0x05,
        Key5        = 0x06,
        Key6        = 0x07,
        Key7        = 0x08,
        Key8        = 0x09,
        Key9        = 0x0A,
        Key0        = 0x0B,
        Minus       = 0x0C,  // - _
        Equals      = 0x0D,  // = +
        Backspace   = 0x0E,

        // Row 3: QWERTY top row
        Tab         = 0x0F,
        Q           = 0x10,
        W           = 0x11,
        E           = 0x12,
        R           = 0x13,
        T           = 0x14,
        Y           = 0x15,
        U           = 0x16,
        I           = 0x17,
        O           = 0x18,
        P           = 0x19,
        LeftBracket = 0x1A,  // [ {
        RightBracket= 0x1B,  // ] }
        Backslash   = 0x2B,  // \ |

        // Row 4: Home row
        CapsLock    = 0x3A,
        A           = 0x1E,
        S           = 0x1F,
        D           = 0x20,
        F           = 0x21,
        G           = 0x22,
        H           = 0x23,
        J           = 0x24,
        K           = 0x25,
        L           = 0x26,
        Semicolon   = 0x27,  // ; :
        Apostrophe  = 0x28,  // ' "
        Enter       = 0x1C,

        // Row 5: Bottom row
        LeftShift   = 0x2A,
        Z           = 0x2C,
        X           = 0x2D,
        C           = 0x2E,
        V           = 0x2F,
        B           = 0x30,
        N           = 0x31,
        M           = 0x32,
        Comma       = 0x33,  // , <
        Period      = 0x34,  // . >
        Slash       = 0x35,  // / ?
        RightShift  = 0x36,

        // Row 6: Bottom modifiers and space
        LeftCtrl    = 0x1D,
        LeftAlt     = 0x38,
        Space       = 0x39,

        // Lock keys
        NumLock     = 0x45,
        ScrollLock  = 0x46,

        // Keypad
        KeypadAsterisk  = 0x37,
        KeypadMinus     = 0x4A,
        KeypadPlus      = 0x4E,
        Keypad7         = 0x47,
        Keypad8         = 0x48,
        Keypad9         = 0x49,
        Keypad4         = 0x4B,
        Keypad5         = 0x4C,
        Keypad6         = 0x4D,
        Keypad1         = 0x4F,
        Keypad2         = 0x50,
        Keypad3         = 0x51,
        Keypad0         = 0x52,
        KeypadPeriod    = 0x53,
    };

    // Extended scancodes (preceded by 0xE0)
    // Use values 0x80+ to distinguish from regular scancodes
    enum class ExtendedScanCode : std::uint8_t {
        Nil             = 0x00,
        
        // The raw byte after 0xE0 prefix
        RightAlt        = 0x38,
        RightCtrl       = 0x1D,
        KeypadSlash     = 0x35,
        KeypadEnter     = 0x1C,

        Insert          = 0x52,
        Delete          = 0x53,
        Home            = 0x47,
        End             = 0x4F,
        PageUp          = 0x49,
        PageDown        = 0x51,

        UpArrow         = 0x48,
        DownArrow       = 0x50,
        LeftArrow       = 0x4B,
        RightArrow      = 0x4D,

        LeftGui         = 0x5B,  // Windows key
        RightGui        = 0x5C,
        Menu            = 0x5D,  // Context menu key
    };

    // Special prefix bytes
    constexpr std::uint8_t EXTENDED_PREFIX = 0xE0;
    constexpr std::uint8_t RELEASE_MASK    = 0x80;
}
