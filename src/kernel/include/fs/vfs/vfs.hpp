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

    kvector<DirEntry> readdir(const char* path);

    void mount(const kstring& path, FileSystem* fs);
}
