#include <fs/devfs/dev_null.hpp>
#include <fs/fs.hpp>

namespace fs::devfs::null {
    static int null_read(FileDescriptor*, void*, std::size_t) {
        return 0;  // EOF
    }

    static int null_write(FileDescriptor*, const void*, std::size_t count) {
        return count;  // Discard, report success
    }

    static int null_close(FileDescriptor*) {
        return 0;
    }

    static int null_lseek(FileDescriptor*, int, int) {
        return 0;
    }

    static int null_fstat(FileDescriptor*, Stat* stat) {
        stat->size = 0;
        stat->type = FileType::CHAR_DEVICE;
        
        return 0;
    }

    static const FileOps null_ops = {
        .read = null_read,
        .write = null_write,
        .close = null_close,
        .lseek = null_lseek,
        .fstat = null_fstat
    };

    static Inode null_inode = {
        .type = FileType::CHAR_DEVICE,
        .size = 0,
        .ops = &null_ops,
        .private_data = nullptr,
    };

    Inode* get_null_inode() {
        return &null_inode;
    }
}
