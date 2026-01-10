#include "console/console.hpp"
#include <fs/devfs/devfs.hpp>
#include <fs/devfs/dev_tty.hpp>
#include <fs/fs.hpp>
#include <fs/vfs/vfs.hpp>

namespace fs::devfs {
    static FileSystem devfs_fs = {
        .name = "devfs",
        .read = nullptr,
        .open = open,
        .readdir = nullptr
    };
    
    void init() {
        vfs::mount("/dev", &devfs_fs);
    }

    Inode open(const kstring& path, int flags) {
        if (path == "/tty1") {
            return {
                .type = FileType::CHAR_DEVICE,
                .size = 0,
                .metadata = nullptr,
                .fs = &devfs_fs,
                .ops = fs::devfs::tty::get_tty_ops()
            };
        }
        
        return vfs::NULL_INODE;
    }
}
