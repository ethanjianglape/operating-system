#pragma once

#include "containers/kstring.hpp"
#include <cstddef>
#include <fs/fs.hpp>
#include <cstdint>

namespace fs::devfs {
    void init();

    Inode open(const kstring& path, int flags);

    std::intmax_t read(FileDescriptor* desc, void* buffer, std::size_t count);
    std::intmax_t write(FileDescriptor* desc, const void* buffer, std::size_t count);
    std::intmax_t close(FileDescriptor* desc);
}
