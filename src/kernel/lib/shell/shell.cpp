#include <containers/kstring.hpp>
#include "algo/string.hpp"
#include "kernel/tty/tty.hpp"
#include <cstddef>
#include <kernel/shell/shell.hpp>
#include <kernel/console/console.hpp>

namespace kernel::shell {
    using RgbColor = kernel::console::RgbColor;
    
    constexpr char ps1[] = "$ >";
    constexpr std::size_t prompt_start = sizeof(ps1) - 1;

    void dispatch_command(const kstring& line) {
        if (line == nullptr) {
            return;
        }

        if (line.empty()) {
            kernel::console::newline();
            return;
        }

        auto split = algo::split(line);

        if (split.empty()) {
            kernel::console::newline();
            return;
        }

        auto command = split[0];

        if (command == "clear") {
            kernel::console::clear();
            return;
        }

        kernel::console::set_color(RgbColor::RED, RgbColor::BLACK);
        kernel::console::newline();
        kernel::console::put("Invalid Command: ");
        kernel::console::put(command);
        kernel::console::reset_color();
        kernel::console::newline();
    }
    
    void init() {
        kernel::tty::init();

        kernel::console::put("Welcome to MyOS!");
        kernel::console::newline();
        
        while (true) {
            kernel::console::disable_cursor();
            kernel::console::put(ps1);
            kernel::console::enable_cursor();
            kernel::console::set_cursor_x(prompt_start);
            const kstring& line = kernel::tty::read_line(prompt_start);
            kernel::console::disable_cursor();
            dispatch_command(line);
            kernel::tty::reset();
        }
    }
}
