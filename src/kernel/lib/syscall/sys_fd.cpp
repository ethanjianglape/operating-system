#include <arch.hpp>
#include <process/process.hpp>
#include <log/log.hpp>
#include <syscall/sys_fd.hpp>
#include <fs/fs.hpp>
#include <fs/vfs.hpp>

#include <cerrno>

namespace syscall {
    static int alloc_fd(process::Process* process) {
        for (std::size_t i = 0; i < process->fd_table.size(); i++) {
            if (process->fd_table[i].inode == nullptr) {
                return i;
            }
        }

        process->fd_table.push_back({});

        return process->fd_table.size() - 1;
    }

    int sys_open(const char* path, int flags) {
        log::debug("sys_open: ", path);

        process::Process* process = arch::percpu::current_process();

        fs::Inode* inode = fs::open(path, flags);

        if (!inode) {
            return -ENOENT;
        }

        int fd = alloc_fd(process);

        process->fd_table[fd].inode = inode;
        process->fd_table[fd].offset = 0;
        process->fd_table[fd].flags = flags;

        return fd;
    }

    int sys_read(int fd, void* buffer, std::size_t count) {
        process::Process* process = arch::percpu::current_process();
        fs::FileDescriptor* desc = &process->fd_table[fd];

        auto ret = desc->inode->ops->read(desc, buffer, count);

        return ret;
    }

    int sys_write(int fd, const void* buffer, std::size_t count) {
        process::Process* process = arch::percpu::current_process();
        fs::FileDescriptor* desc = &process->fd_table[fd];

        return desc->inode->ops->write(desc, buffer, count);
    }

    int sys_close(int fd) {
        process::Process* process = arch::percpu::current_process();
        fs::FileDescriptor* desc = &process->fd_table[fd];

        int result = desc->inode->ops->close(desc);

        process->fd_table[fd] = {};

        return result;
    }

    int sys_fstat(int fd, fs::Stat* stat) {
        process::Process* process = arch::percpu::current_process();
        fs::FileDescriptor* desc = &process->fd_table[fd];

        return 0;

        //return desc->inode->ops->stat(desc, stat);
    }
}
