#pragma once

#include <containers/kstring.hpp>
#include <containers/kvector.hpp>

#include <cstddef>
#include <cstdint>

namespace fs {
    enum class FileType : std::uint8_t {
        NOT_FOUND = 0,
        FILE = 1,
        DIRECTORY = 2,
        CHAR_DEVICE = 3
    };

    constexpr int EOF = 0;
    constexpr int O_RDONLY = 0x01;

    struct Inode {
        FileType type;
        std::size_t size;
        void* metadata;
        struct FileSystem* fs;
        struct FileOps* ops;
    };

    struct FileDescriptor {
        Inode inode;
        std::size_t offset;
        int flags;
    };

    struct DirEntry {
        FileType type;
        std::size_t size;
        kstring name;
    };

    struct Stat {
        FileType type;
        std::size_t size;
        kstring path;
    };

    using filesystem_read_fn = std::intmax_t (*)(Inode* inode, void* buffer, std::size_t count, std::size_t offset);
    using filesystem_open_fn = Inode (*)(const kstring& path, int flags);
    using filesystem_readdir_fn = kvector<DirEntry> (*)(const kstring& cpath);
    
    struct FileSystem {
        kstring name;
        filesystem_read_fn read;
        filesystem_open_fn open;
        filesystem_readdir_fn readdir;
    };

    using fileops_read_fn  = std::intmax_t (*)(FileDescriptor* desc, void* buffer, std::size_t count);
    using fileops_write_fn = std::intmax_t (*)(FileDescriptor* desc, const void* buffer, std::size_t count);
    using fileops_stat_fn  = std::intmax_t (*)(FileDescriptor* desc, Stat* stat);
    using fileops_close_fn = std::intmax_t (*)(FileDescriptor* desc);

    struct FileOps {
        fileops_read_fn read;
        fileops_write_fn write;
        fileops_stat_fn stat;
        fileops_close_fn close;
    };
}
