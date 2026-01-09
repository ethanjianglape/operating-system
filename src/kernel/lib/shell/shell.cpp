#include "containers/kvector.hpp"
#include "fmt/fmt.hpp"
#include "fs/fs.hpp"
#include "log/log.hpp"
#include "memory/memory.hpp"
#include "syscall/fd/syscall_fd.hpp"
#include <algo/algo.hpp>
#include <tty/tty.hpp>
#include <cstddef>
#include <shell/shell.hpp>
#include <console/console.hpp>
#include <process/process.hpp>
#include <fs/vfs/vfs.hpp>

namespace shell {
    using RgbColor = console::RgbColor;
    
    static kstring pwd;

    void dispatch_command(const kstring& line) {
        if (line.empty()) {
            console::newline();
            return;
        }

        auto split = algo::split(line);

        if (split.empty()) {
            console::newline();
            return;
        }

        auto command = split[0];

        log::debug("Shell parsing command: ", command.c_str());

        if (command == "pwd") {
            console::newline();
            console::put(pwd);
            console::newline();
            return;
        }

        if (command == "clear") {
            console::clear();
            return;
        }

        if (command == "ls") {
            auto filename = split.size() > 1 ? split[1] : pwd;
            
            kvector<fs::DirEntry> entries = {};//fs::readdir(filename.c_str());

            for (const auto& entry : entries) {
                if (entry.type == fs::FileType::DIRECTORY) {
                    console::set_color(RgbColor::CYAN, RgbColor::BLACK);
                }

                console::newline();
                console::put(entry.name);

                if (entry.type == fs::FileType::DIRECTORY) {
                    console::reset_color();
                }
            }

            console::newline();

            return;
        }

        if (command == "cd") {
            if (split.size() == 1) {
                pwd = "/";
                console::newline();
                return;
            }
            
            auto filename = split[1];

            if (filename.front() != '/') {
                filename = pwd + "/" + filename;
            }

            int fd = syscall::fd::sys_open(filename.c_str(), fs::O_RDONLY);
            fs::Stat stat;

            syscall::fd::sys_fstat(fd, &stat);

            if (stat.type == fs::FileType::DIRECTORY) {
                pwd = stat.path;
                console::newline();
            } else {
                console::set_color(RgbColor::RED, RgbColor::BLACK);
                console::newline();
                console::put("Not a directory: ");
                console::put(stat.path);
                console::reset_color();
                console::newline();
            }
            
            return;
        }

        if (command == "run") {
            auto filename = split[1];

            if (filename[0] != '/'){
                filename = pwd + "/" + filename;
            }

            int fd = syscall::fd::sys_open(filename.c_str(), fs::O_RDONLY);

            if (fd >= 0) {
                fs::Stat stat;
                syscall::fd::sys_fstat(fd, &stat);

                auto* buffer = kalloc<std::uint8_t>(stat.size);

                syscall::fd::sys_read(fd, buffer, stat.size);

                process::load(buffer, stat.size);

                syscall::fd::sys_close(fd);
                kfree(buffer);
            }

            console::newline();
            return;
        }

        if (command == "cat") {
            auto filename = split[1];

            if (filename[0] != '/'){
                filename = pwd + "/" + filename;
            }

            int fd = syscall::fd::sys_open(filename.c_str(), fs::O_RDONLY);

            if (fd >= 0) {
                char buffer[512];
                int read = syscall::fd::sys_read(fd, buffer, 512);

                log::debug("read = ", read);
                
                kstring file{buffer, read};

                log::debug(file);

                console::newline();
                console::put(file);
                console::newline();

                syscall::fd::sys_close(fd);
            } else {
                console::set_color(RgbColor::RED, RgbColor::BLACK);
                console::newline();
                console::put("File not found: ");
                console::put(filename);
                console::reset_color();
                console::newline();
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
        int fd = syscall::fd::sys_open("/dev/tty1", fs::O_RDONLY);

        const char* prompt = "> $";
        char buffer[512];

        while (true) {
            syscall::fd::sys_write(fd, prompt, 3);

            int n = syscall::fd::sys_read(fd, buffer, sizeof(buffer) - 1);

            if (n > 0) {
                buffer[n] = '\0';

                kstring line{buffer};

                syscall::fd::sys_write(fd, "command: ", 10);
                syscall::fd::sys_write(fd, buffer, n);
                syscall::fd::sys_write(fd, "\n", 1);

                log::debug("console read: ", line);
            }
        }
        
        /*
        tty::init();

        pwd = "/";

        console::put("Welcome to MyOS!");
        console::newline();

        while (true) {
            kstring ps1 = pwd + " >";
            std::size_t prompt_start = ps1.size();

            console::disable_cursor();
            console::put(ps1);
            console::enable_cursor();
            console::redraw();
            
            const kstring& line = tty::read_line(prompt_start);
            console::disable_cursor();
            dispatch_command(line);
            tty::reset();
        }
        */
    }
}
