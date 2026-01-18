#include <cstdint>
#include <fs/devfs/dev_random.hpp>
#include <timer/timer.hpp>

namespace fs::devfs::random {
    static int random_read(FileDescriptor*, void* buff, std::size_t count) {
        auto* ptr = reinterpret_cast<std::uint64_t*>(buff);

        for (std::size_t i = 0; i < count; i++) {
            *(ptr + i) = timer::get_ticks() + i * i;
        }

        return 0;
    }

    static int random_write(FileDescriptor*, const void*, std::size_t count) {
        return count;  // Discard, report success
    }

    static int random_close(FileDescriptor*) {
        return 0;
    }

    static int random_lseek(FileDescriptor*, int, int) {
        return 0;
    }

    static const FileOps random_ops = {
        .read = random_read,
        .write = random_write,
        .close = random_close,
        .lseek = random_lseek,
    };

    static Inode random_inode = {
        .type = FileType::CHAR_DEVICE,
        .size = 0,
        .ops = &random_ops,
        .private_data = nullptr,
    };

    Inode* get_random_inode() {
        return &random_inode;
    }
}
