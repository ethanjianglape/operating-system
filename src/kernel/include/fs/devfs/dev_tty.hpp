#pragma once

#include <fs/fs.hpp>

namespace process { struct Process; }

namespace fs::devfs::tty {
    Inode* get_tty_inode();

    process::Process* get_waiting_process();

    void init();
}
