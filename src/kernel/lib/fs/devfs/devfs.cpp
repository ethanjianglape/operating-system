#include <fs/devfs/devfs.hpp>
#include <fs/devfs/dev_tty.hpp>
#include <fs/devfs/dev_null.hpp>
#include <fs/devfs/dev_random.hpp>
#include <fs/fs.hpp>

namespace fs::devfs {
    static Inode* devfs_open(FileSystem* self, const kstring& path, int flags);
    static int devfs_stat(FileSystem* self, const kstring& path, Stat* out);
    static int devfs_readdir(FileSystem* self, const kstring& path, kvector<DirEntry>& out);

    static FileSystem devfs_fs = {
        .name = "devfs",
        .private_data = nullptr,
        .open = devfs_open,
        .stat = devfs_stat,
        .readdir = devfs_readdir,
    };

    void init() {
        fs::mount("/dev", &devfs_fs);
    }

    static Inode* devfs_open(FileSystem*, const kstring& path, int) {
        if (path == "/tty1") {
            return tty::get_tty_inode();
        }
        
        if (path == "/null") {
            return null::get_null_inode();
        }

        if (path == "/random") {
            return random::get_random_inode();
        }

        return nullptr;
    }

    static int devfs_stat(FileSystem*, const kstring& path, Stat* out) {
        if (path == "/tty1" || path == "/null") {
            out->type = FileType::CHAR_DEVICE;
            out->size = 0;
            return 0;
        }

        return -1;
    }

    static int devfs_readdir(FileSystem*, const kstring& path, kvector<DirEntry>& out) {
        if (path.empty()) {
            out.push_back(DirEntry{
                .name = "tty1",
                .type = FileType::CHAR_DEVICE,
            });
            
            out.push_back(DirEntry{
                .name = "null",
                .type = FileType::CHAR_DEVICE,
            });
            
            return 0;
        }

        return -1;
    }
}
