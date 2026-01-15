#include <fs/devfs/dev_null.hpp>
#include <fs/fs.hpp>

namespace fs::devfs::null {
    static std::intmax_t null_read(FileDescriptor*, void*, std::size_t) {
        return 0;  // EOF
    }

    static std::intmax_t null_write(FileDescriptor*, const void*, std::size_t count) {
        return count;  // Discard, report success
    }

    static std::intmax_t null_close(FileDescriptor*) {
        return 0;
    }

    static const FileOps null_ops = {
        .read = null_read,
        .write = null_write,
        .close = null_close,
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
