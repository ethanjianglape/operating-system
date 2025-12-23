#include "kernel/tty/tty.hpp"
#include <cstddef>
#include <kernel/shell/shell.hpp>
#include <kernel/console/console.hpp>
#include <string.h>

namespace kernel::shell {
    using RgbColor = kernel::console::RgbColor;
    
    constexpr char ps1[] = "$ >";
    constexpr std::size_t prompt_start = sizeof(ps1) - 1;

    void dispatch_command(char* line) {
        if (line == nullptr) {
            return;
        }

        if (strlen(line) == 0) {
            kernel::console::newline();
            return;
        }

        if (strcmp(line, "clear") == 0) {
            kernel::console::clear();
            return;
        }

        kernel::console::set_color(RgbColor::RED, RgbColor::BLACK);
        kernel::console::newline();
        kernel::console::put("Invalid Command: ");
        kernel::console::put(line);
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
            char* line = kernel::tty::read_line(prompt_start);
            kernel::console::disable_cursor();
            dispatch_command(line);
            kernel::tty::reset();
        }
    }
}
