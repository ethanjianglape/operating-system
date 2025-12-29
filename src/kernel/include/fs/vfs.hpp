#pragma once

#include "containers/kvector.hpp"
#include <containers/kstring.hpp>
#include <cstddef>
#include <cstdint>

namespace fs {
    enum class FileType : std::uint8_t {
        NOT_FOUND = 0,
        FILE = 1,
        DIRECTORY = 2
    };

    constexpr int EOF = 0;
    constexpr int O_RDONLY = 0x01;

    struct Inode {
        FileType type;
        std::size_t size;
        void* metadata;
        struct FileSystem* fs;
    };

    struct MountPoint {
        kstring root;
        FileSystem* fs;
    };

    struct Stat {
        FileType type;
        std::size_t size;
        kstring path;
    };

    struct DirEntry {
        FileType type;
        std::size_t size;
        kstring name;
    };

    struct FileDescriptor {
        Inode inode;
        std::size_t offset;
        int flags;
    };

    using filesystem_read_fn = std::size_t (*)(Inode* inode, void* buffer, std::size_t count, std::size_t offset);
    using filesystem_open_fn = Inode (*)(const char* path, int flags);
    using filesystem_readdir_fn = kvector<DirEntry> (*)(const kstring& path);

    struct FileSystem {
        kstring name;
        filesystem_read_fn read;
        filesystem_open_fn open;
        filesystem_readdir_fn readdir;
    };

    constexpr Inode NULL_INODE = {};

    int open(const char* path, int flags);
    int close(int fd);
    int read(int fd, void* buffer, std::size_t count);
    Stat stat(const char* path);
    kvector<DirEntry> readdir(const char* path);

    void mount(const kstring& path, FileSystem* fs);
}
