#pragma once

#include <containers/kvector.hpp>
#include <fs/fs.hpp>

namespace syscall {
    int sys_open(const char* path, int flags);
    int sys_read(int fd, void* buffer, std::size_t count);
    int sys_write(int fd, const void* buffer, std::size_t count);
    int sys_close(int fd);
    int sys_stat(const char* path, fs::Stat* stat);
    int sys_fstat(int fd, fs::Stat* stat);
    int sys_lseek(int fd, std::size_t offset, int whence);
    int sys_getcwd(char* buffer, std::size_t size);
    int sys_chdir(const char* buffer, std::size_t size);
}
