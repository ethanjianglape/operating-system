#include "fs/vfs.hpp"
#include "log/log.hpp"
#include <algo/algo.hpp>
#include <tty/tty.hpp>
#include <cstddef>
#include <shell/shell.hpp>
#include <console/console.hpp>

namespace shell {
    using RgbColor = console::RgbColor;
    
    constexpr char ps1[] = "$ >";
    constexpr std::size_t prompt_start = sizeof(ps1) - 1;

    void dispatch_command(const kstring& line) {
        if (line == nullptr) {
            return;
        }

        if (line.empty()) {
            console::newline();
            return;
        }

        auto split = algo::split(line);

        for (const auto& s : split) {
            log::debug(s);
        }

        if (split.empty()) {
            console::newline();
            return;
        }

        auto command = split[0];

        if (command == "clear") {
            console::clear();
            return;
        }

        if (command == "cat") {
            auto filename = split[1];

            log::debug(filename);
            log::debug(filename.c_str());

            int fd = fs::open(filename.c_str(), fs::O_RDONLY);

            log::debug("fd = ", fd);

            if (fd >= 0) {
                char buffer[512];
                int read = fs::read(fd, buffer, 512);

                log::debug("read = ", read);
                
                kstring file{buffer, read};

                log::debug(file);

                console::newline();
                console::put(file);
                console::newline();

                fs::close(fd);
            }

            return;
        }

        console::set_color(RgbColor::RED, RgbColor::BLACK);
        console::newline();
        console::put("Invalid Command: ");
        console::put(command);
        console::reset_color();
        console::newline();
    }
    
    void init() {
        tty::init();

        console::put("Welcome to MyOS!");
        console::newline();

        while (true) {
            console::disable_cursor();
            console::put(ps1);
            console::enable_cursor();
            console::set_cursor_x(prompt_start);
            const kstring& line = tty::read_line(prompt_start);
            console::disable_cursor();
            dispatch_command(line);
            tty::reset();
        }
    }
}
