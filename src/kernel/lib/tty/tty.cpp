#include "arch/x86_64/drivers/keyboard/keyboard.hpp"
#include "arch/x86_64/drivers/keyboard/scancodes.hpp"
#include "kernel/console/console.hpp"
#include "kernel/drivers/framebuffer/framebuffer.hpp"
#include <kernel/tty/tty.hpp>
#include <kernel/arch/arch.hpp>

namespace kernel::tty {
    namespace keyboard = arch::drivers::keyboard;
    using ScanCode = keyboard::ScanCode;

    static char input[512];

    // Scancode Set 1 to ASCII lookup table (lowercase, unshifted)
    // Index = scancode value, value = ASCII char (0 = non-printable)
    static constexpr char scancode_to_ascii_table[128] = {
        0,    0,   '1', '2', '3', '4', '5', '6',   // 0x00-0x07
        '7', '8', '9', '0', '-', '=',  0,   0,     // 0x08-0x0F (0x0E=Backspace, 0x0F=Tab)
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',    // 0x10-0x17
        'o', 'p', '[', ']',  0,   0,  'a', 's',    // 0x18-0x1F (0x1C=Enter, 0x1D=LCtrl)
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',    // 0x20-0x27
        '\'', '`', 0,  '\\', 'z', 'x', 'c', 'v',   // 0x28-0x2F (0x2A=LShift)
        'b', 'n', 'm', ',', '.', '/',  0,  '*',    // 0x30-0x37 (0x36=RShift, 0x37=Keypad*)
         0,  ' ',  0,   0,   0,   0,   0,   0,     // 0x38-0x3F (0x38=LAlt, 0x39=Space, 0x3A=Caps, 0x3B-0x3F=F1-F5)
         0,   0,   0,   0,   0,   0,   0,  '7',    // 0x40-0x47 (0x40-0x44=F6-F10, 0x45=Num, 0x46=Scroll, 0x47=Kp7)
        '8', '9', '-', '4', '5', '6', '+', '1',    // 0x48-0x4F (Keypad)
        '2', '3', '0', '.',  0,   0,   0,   0,     // 0x50-0x57 (Keypad, 0x57=F11)
         0,   0,   0,   0,   0,   0,   0,   0,     // 0x58-0x5F (0x58=F12)
         0,   0,   0,   0,   0,   0,   0,   0,     // 0x60-0x67
         0,   0,   0,   0,   0,   0,   0,   0,     // 0x68-0x6F
         0,   0,   0,   0,   0,   0,   0,   0,     // 0x70-0x77
         0,   0,   0,   0,   0,   0,   0,   0,     // 0x78-0x7F
    };

    char scancode_to_ascii(ScanCode code, bool caps) {
        auto index = static_cast<std::uint8_t>(code);
        
        if (index >= 128) {
            return 0;
        }

        char ascii = scancode_to_ascii_table[index];

        if (caps && ascii >= 'a' && ascii <= 'z') {
            return ascii - 32;
        }
        
        return ascii;
    }

    void loop() {
        while (keyboard::KeyEvent* event = keyboard::read()) {
            keyboard::ScanCode scancode = event->scancode;
            keyboard::ExtendedScanCode extended = event->extended_scancode;

            if (event->released) {
                continue;
            }

            char ascii = scancode_to_ascii(scancode, event->shift_held);

            if (ascii > 0) {
                kernel::console::put(ascii);
            } else if (scancode == ScanCode::Backspace) {
                kernel::console::backspace();
            } else if (scancode == ScanCode::Enter) {
                kernel::console::newline();
            }
        }
    }

    void init() {
        kernel::console::init(kernel::drivers::framebuffer::get_console_driver());

        loop();
    }
}
