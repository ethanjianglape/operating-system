#pragma once

#include <containers/kvector.hpp>
#include <fs/fs.hpp>

namespace syscall::fd {
    int sys_open(const char* path, int flags);
    int sys_read(int fd, void* buffer, std::size_t count);
    int sys_write(int fd, const void* buffer, std::size_t count);
    int sys_close(int fd);
    int sys_fstat(int fd, fs::Stat* stat);
}
