#pragma once

#include <fs/fs.hpp>

namespace process { struct Process; }

namespace fs::devfs::tty {
    Inode* get_tty_inode();

    void init();
}
