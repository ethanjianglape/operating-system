#pragma once

#include <fs/fs.hpp>

#include <cstddef>
#include <cstdint>

namespace fs::initramfs {
    void init(std::uint8_t* addr, std::size_t size);

    std::intmax_t read(Inode* inode, void* buffer, std::size_t count, std::size_t offset);
    Inode open(const kstring& path, int flags);
    kvector<DirEntry> readdir(const kstring& path);
}
