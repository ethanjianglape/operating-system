#pragma once

#include <fs/fs.hpp>

namespace fs::devfs::tty {
    FileOps* get_tty_ops();
    
    std::intmax_t read(FileDescriptor* desc, void* buffer, std::size_t count);
    std::intmax_t write(FileDescriptor* desc, const void* buffer, std::size_t count);
    std::intmax_t close(FileDescriptor* desc);
}
