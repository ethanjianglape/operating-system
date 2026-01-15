#pragma once

#include <fs/fs.hpp>
#include <containers/kvector.hpp>
#include <containers/kstring.hpp>

namespace fs {
    struct MountPoint {
        kstring root;
        FileSystem* filesystem;
    };

    // Mount a filesystem at a path (e.g., mount("/", &initramfs))
    void mount(const kstring& path, FileSystem* fs);

    // Open a file by path, returns heap-allocated Inode or nullptr
    Inode* open(const kstring& path, int flags);

    // Stat a path without opening
    int stat(const kstring& path, Stat* out);

    // List directory contents
    int readdir(const kstring& path, kvector<DirEntry>& out);
}
