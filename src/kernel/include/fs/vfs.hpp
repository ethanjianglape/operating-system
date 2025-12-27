#pragma once

#include <containers/kstring.hpp>
#include <cstddef>

namespace fs {
    enum class FileType { File = 1, Dir = 2 };

    constexpr int EOF = 0;
    constexpr int O_RDONLY = 0x01;

    struct Inode {
        FileType type;
        std::size_t size;
        void* metadata;
        struct FileSystem* fs;
    };

    using filesystem_read_fn = std::size_t (*)(Inode* inode, void* buffer, std::size_t count, std::size_t offset);
    using filesystem_open_fn = Inode (*)(const char* path, int flags);

    struct FileDescriptor {
        Inode inode;
        std::size_t offset;
        int flags;
    };

    struct FileSystem {
        kstring name;
        filesystem_read_fn read;
        filesystem_open_fn open;
    };

    struct MountPoint {
        kstring path;
        FileSystem* fs;
    };

    constexpr Inode NULL_INODE = {};

    int open(const char* path, int flags);
    int close(int fd);
    int read(int fd, void* buffer, std::size_t count);

    void mount(const kstring& path, FileSystem* fs);
}
