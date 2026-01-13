#include "containers/kvector.hpp"
#include <arch.hpp>
#include <console/console.hpp>
#include <containers/kstring.hpp>
#include <log/log.hpp>
#include <tty/tty.hpp>

#include <cstddef>
#include <cstdint>

namespace tty {
    namespace keyboard = arch::drivers::keyboard;
    
    using ScanCode = keyboard::ScanCode;
    using ExtendedScanCode = keyboard::ExtendedScanCode;

    //constexpr std::size_t BUFFER_LEN = 512;
    //static char buffer[BUFFER_LEN] = {'\0'};

    static kstring buffer{};
    static kvector<kstring> history{};

    static std::size_t buffer_index = 0;
    static std::size_t history_index = 0;

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

    void insert_char(char c) {
        buffer.insert(buffer_index, c);
        buffer_index++;
    }

    void delete_back() {
        if (buffer_index == 0) {
            return;
        }

        buffer.erase(buffer_index - 1);
        buffer_index--;
    }

    void delete_forward() {
        if (buffer_index == buffer.size()) {
            return;
        }

        buffer.erase(buffer_index);
    }

    void move_left() {
        if (buffer_index > 0) {
            buffer_index--;
        }
    }

    void move_right() {
        if (buffer_index < buffer.size()) {
            buffer_index++;
        }
    }

    void move_to_start() {
        buffer_index = 0;
    }

    void move_to_end() {
        buffer_index = buffer.size();
    }

    void delete_to_end() {
        buffer.truncate(buffer_index);
    }

    void add_buffer_history() {
        // do not add empty string or back to back duplicates to history
        if (buffer.empty()) {
            return;
        }

        if (!history.empty() && buffer == history.back()) {
            return;
        }
        
        history.push_back(buffer);
        history_index = history.size();
    }

    void buffer_history_up() {
        if (!history.empty() && history_index > 0) {
            buffer = history[--history_index];
            buffer_index = buffer.size();
        }
    }

    void buffer_history_down() {
        if (history_index + 1 < history.size()) {
            buffer = history[++history_index];
            buffer_index = buffer.size();
        } else {
            buffer = "";
            buffer_index = buffer.size();
            history_index = history.size();
        }
    }

    void process_ctrl(keyboard::ScanCode c, keyboard::ExtendedScanCode extended) {
        if (c == ScanCode::A) {
            move_to_start();
        } else if (c == ScanCode::E) {
            move_to_end();
        } else if (c == ScanCode::K) {
            delete_to_end();
        } else if (c == ScanCode::B) {
            move_left();
        } else if (c == ScanCode::F) {
            move_right();
        } else if (c == ScanCode::D) {
            delete_forward();
        }
    }

    void page_up() {
        console::scroll_up();
    }

    void page_down() {
        console::scroll_down();
    }

    const kstring& read_line(std::size_t prompt_start) {
        while (true) {
            while (keyboard::KeyEvent* event = keyboard::read()) {
                console::enable_cursor();
                
                keyboard::ScanCode scancode = event->scancode;
                keyboard::ExtendedScanCode extended = event->extended_scancode;

                if (event->released) {
                    continue;
                }

                bool caps = event->shift_held || event->caps_lock_on;
                bool ctrl = event->control_held;

                char ascii = scancode_to_ascii(scancode, caps);

                if (ctrl) {
                    process_ctrl(scancode, extended);
                } else if (ascii != '\0') {
                    insert_char(ascii);
                } else if (scancode == ScanCode::Backspace) {
                    delete_back();
                } else if (scancode == ScanCode::Enter) {
                    add_buffer_history();
                    
                    return buffer;
                } else if (extended == ExtendedScanCode::LeftArrow) {
                    move_left();
                } else if (extended == ExtendedScanCode::RightArrow) {
                    move_right();
                } else if (extended == ExtendedScanCode::Delete) {
                    delete_forward();
                } else if (extended == ExtendedScanCode::UpArrow) {
                    buffer_history_up();
                } else if (extended == ExtendedScanCode::DownArrow) {
                    buffer_history_down();
                } else if (extended == ExtendedScanCode::PageUp) {
                    page_up();
                } else if (extended == ExtendedScanCode::PageDown) {
                    page_down();
                }

                console::disable_cursor();
                console::set_cursor_x(prompt_start);
                console::put(buffer);
                console::clear_to_eol();
                console::set_cursor_x(prompt_start + buffer_index);
                console::enable_cursor();
                console::redraw();
            }
        }
    }

    void reset() {
        buffer = "";
        buffer_index = 0;
    }

    void init() {
        console::init();

        reset();
    }
}
