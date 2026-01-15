#include <arch.hpp>
#include "process/process.hpp"
#include <log/log.hpp>
#include <syscall/fd/syscall_fd.hpp>
#include <fs/vfs/vfs.hpp>
#include <fs/fs.hpp>
#include <containers/kvector.hpp>
#include <containers/kstring.hpp>

#include <cerrno>

namespace syscall::fd {
    int alloc_fd(process::Process* process) {
        for (std::size_t i = 0; i < process->fd_table.size(); i++) {
            if (process->fd_table[i].inode.metadata == nullptr) {
                return i;
            }
        }

        process->fd_table.push_back({});

        return process->fd_table.size() - 1;
    }

    int sys_open(const char* path, int flags) {
        log::debug("sys_open: ", path);

        arch::entry::PerCPU* per_cpu = arch::entry::get_per_cpu();
        process::Process* process = per_cpu->process;
        
        fs::Inode inode = fs::vfs::lookup(path, flags);

        if (inode.type == fs::FileType::NOT_FOUND) {
            return -ENOENT;
        }

        int fd = alloc_fd(process);

        process->fd_table[fd].inode = inode;
        process->fd_table[fd].offset = 0;
        process->fd_table[fd].flags = flags;

        return fd;
    }

    int sys_read(int fd, void* buffer, std::size_t count) {
        arch::entry::PerCPU* per_cpu = arch::entry::get_per_cpu();
        process::Process* process = per_cpu->process;
        fs::FileDescriptor* desc = &process->fd_table[fd];

        auto ret = desc->inode.ops->read(desc, buffer, count);

        return ret;
    }

    int sys_write(int fd, const void* buffer, std::size_t count) {
        arch::entry::PerCPU* per_cpu = arch::entry::get_per_cpu();
        process::Process* process = per_cpu->process;
        fs::FileDescriptor* desc = &process->fd_table[fd];

        return desc->inode.ops->write(desc, buffer, count);
    }

    int sys_close(int fd) {
        arch::entry::PerCPU* per_cpu = arch::entry::get_per_cpu();
        process::Process* process = per_cpu->process;
        fs::FileDescriptor* desc = &process->fd_table[fd];

        int result = desc->inode.ops->close(desc);

        process->fd_table[fd] = {};

        return result;
    }

    int sys_fstat(int fd, fs::Stat* stat) {
        arch::entry::PerCPU* per_cpu = arch::entry::get_per_cpu();
        process::Process* process = per_cpu->process;
        fs::FileDescriptor* desc = &process->fd_table[fd];

        return desc->inode.ops->stat(desc, stat);
    }
}
