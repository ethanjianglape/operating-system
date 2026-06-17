#pragma once

#include "arch/x64/drivers/keyboard/ps2.hpp"
#include "containers/kstring.hpp"
#include <arch.hpp>
#include <fs/fs.hpp>

namespace process {
struct Process;
}

namespace fs::devfs {
namespace keyboard = arch::drivers::keyboard;

class DevTtyInode final : public Inode {
private:
    kstring buffer{};
    kvector<kstring> history{};

    std::size_t buffer_index = 0;
    std::size_t history_index = 0;

    void page_up();
    void page_down();
    void insert_char(char c);
    void delete_back();
    void delete_forward();
    void move_left();
    void move_right();
    void move_to_start();
    void move_to_end();
    void delete_to_end();
    void add_buffer_history();
    void buffer_history_up();
    void buffer_history_down();
    void process_ctrl(keyboard::ScanCode c, keyboard::ExtendedScanCode ex);

public:
    DevTtyInode();

    int open(FileDescriptor* fd, int flags) override;
    int read(FileDescriptor* fd, void* buf, std::size_t count) override;
    int write(FileDescriptor* fd, const void* buf, std::size_t count) override;
    int close(FileDescriptor* fd) override;
    int lseek(FileDescriptor* fd, int offset, int whence) override;
    int stat(Stat* stat) override;
};

void init_tty();
}
