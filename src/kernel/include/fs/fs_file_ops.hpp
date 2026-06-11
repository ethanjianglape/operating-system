#pragma once

#include <cstdint>
#include <fs/fs.hpp>

namespace fs {
// Metadata for filesystem files (stored in inode->private_data)
struct FsFileMeta {
    const std::uint8_t* data;
    std::size_t         size;
};

// Shared FileOps for filesystem files (initramfs, future disk filesystems)
const FileOps* get_fs_file_ops();
}
