#include "arch/x86_64/entry/entry.hpp"
#include "fmt/fmt.hpp"
#include "fs/fs.hpp"
#include "fs/initramfs/initramfs.hpp"
#include "fs/vfs/vfs.hpp"
#include "log/log.hpp"
#include "process/process.hpp"
#include "scheduler/scheduler.hpp"
#include <fs/devfs/dev_tty.hpp>
#include <console/console.hpp>
#include <containers/kvector.hpp>
#include <containers/kstring.hpp>
#include <crt/crt.h>
#include <arch.hpp>

namespace fs::devfs::tty {
    namespace keyboard = arch::drivers::keyboard;
    
    using ScanCode = keyboard::ScanCode;
    using ExtendedScanCode = keyboard::ExtendedScanCode;

    static kstring buffer{};
    static kvector<kstring> history{};

    static std::size_t buffer_index = 0;
    static std::size_t history_index = 0;
    
    static FileOps tty_ops = {
        .read = read,
        .write = write,
        .stat = nullptr,
        .close = close
    };

    static process::Process* waiting_process;

    process::Process* get_waiting_process() { return waiting_process; }

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

        console::erase_in_line(console::get_cursor_x(), console::get_screen_cols());
        console::put(c);

        for (std::size_t i = buffer_index; i < buffer.size(); i++) {
            console::put(buffer[i]);
        }

        int len = buffer.size() - buffer_index;

        console::move_cursor(-len, 0);
    }

    void delete_back() {
        if (buffer_index == 0) {
            return;
        }

        buffer.erase(buffer_index - 1);
        buffer_index--;

        console::move_cursor(-1, 0);
        console::erase_in_line(console::get_cursor_x(), console::get_screen_cols());

        for (std::size_t i = buffer_index; i < buffer.size(); i++) {
            console::put(buffer[i]);
        }

        int len = buffer.size() - buffer_index;

        console::move_cursor(-len, 0);
    }

    void delete_forward() {
        if (buffer_index == buffer.size()) {
            return;
        }

        buffer.erase(buffer_index);

        console::erase_in_line(console::get_cursor_x(), console::get_screen_cols());

        for (std::size_t i = buffer_index; i < buffer.size(); i++) {
            console::put(buffer[i]);
        }

        int len = buffer.size() - buffer_index;

        console::move_cursor(-len, 0);
    }

    void move_left() {
        if (buffer_index > 0) {
            buffer_index--;
            console::move_cursor(-1, 0);
        }
    }

    void move_right() {
        if (buffer_index < buffer.size()) {
            buffer_index++;
            console::move_cursor(1, 0);
        }
    }

    void move_to_start() {
        if (buffer_index == 0) {
            return;
        }

        console::move_cursor(-buffer_index, 0);
        buffer_index = 0;
    }

    void move_to_end() {
        if (buffer_index == buffer.size()) {
            return;
        }

        std::size_t len = buffer.size() - buffer_index;
        console::move_cursor(len, 0);
        
        buffer_index = buffer.size();
    }

    void delete_to_end() {
        buffer.truncate(buffer_index);
        console::erase_in_line(console::get_cursor_x(), console::get_screen_cols());
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
            move_to_start();
            delete_to_end();
            
            buffer = history[--history_index];
            buffer_index = buffer.size();

            console::put(buffer);
        }
    }

    void buffer_history_down() {
        move_to_start();
        delete_to_end();
        
        if (history_index + 1 < history.size()) {
            buffer = history[++history_index];
            buffer_index = buffer.size();
        } else {
            buffer = "";
            buffer_index = buffer.size();
            history_index = history.size();
        }

        console::put(buffer);
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

    FileOps* get_tty_ops() { return &tty_ops; }

    void run_tty_program(const kstring& name) {
        Inode inode = vfs::lookup(name.c_str(), fs::O_RDONLY);
        std::uint8_t* data = new std::uint8_t[inode.size];
        initramfs::read(&inode, data, inode.size, 0);
        process::Process* p = process::create_process(data, inode.size);

        p->fd_table.push_back({});
        p->fd_table.push_back({});
        p->fd_table.push_back({});

        p->fd_table[0].inode = vfs::lookup("/dev/tty1", fs::O_RDONLY); // stdin
        p->fd_table[1].inode = vfs::lookup("/dev/tty1", fs::O_RDONLY); // stdout
        p->fd_table[2].inode = vfs::lookup("/dev/tty1", fs::O_RDONLY); // stderr

        scheduler::add_process(p);
    }

    void init() {
        log::init_start("/dev/tty");

        //run_tty_program("/bin/hello");
        run_tty_program("/bin/a");
        run_tty_program("/bin/b");
        run_tty_program("/bin/c");

        log::init_end("/dev/tty");
    }
    
    std::intmax_t read(FileDescriptor* desc, void* buff, std::size_t count) {
        auto* per_cpu = x86_64::entry::get_per_cpu();
        auto* process = per_cpu->process;

        waiting_process = process;
        buffer = "";
        buffer_index = 0;

        while (true) {
            while (keyboard::KeyEvent* event = keyboard::poll()) {
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

                    waiting_process = nullptr;
                    std::size_t len = buffer.size();
                    memcpy(buff, buffer.c_str(), len > count ? count : len);
                    console::newline();
                    log::debug("/dev/tty returning: ", buffer);
                    
                    return len;
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

                console::redraw();
            }

            log::debug("/dev/tty yielding");
            scheduler::yield_blocked(waiting_process);
        }
    }

    std::intmax_t write(FileDescriptor* desc, const void* buffer, std::size_t count) {
        const auto* cbuffer = reinterpret_cast<const char*>(buffer);
        kstring str(cbuffer, count);

        console::put(str);
        console::redraw();

        return str.size();
    }

    std::intmax_t close(FileDescriptor* desc) {
        return 0;
    }
}
