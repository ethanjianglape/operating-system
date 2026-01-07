#pragma once

#include <fs/fs.hpp>
#include "containers/kvector.hpp"
#include <containers/kstring.hpp>
#include <cstddef>

namespace fs::vfs {
    struct MountPoint {
        kstring root;
        FileSystem* fs;
    };

    constexpr Inode NULL_INODE = {};

    FileOps* get_vfs_file_ops();

    Inode lookup(const char* path, int flags);

    int open(const char* path, int flags);
    int close(int fd);
    int read(int fd, void* buffer, std::size_t count);
    Stat stat(const char* path);
    kvector<DirEntry> readdir(const char* path);

    void mount(const kstring& path, FileSystem* fs);
}
