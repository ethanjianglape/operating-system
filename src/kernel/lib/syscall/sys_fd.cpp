#include "containers/kstring.hpp"
#include <arch.hpp>
#include <cstddef>
#include <process/process.hpp>
#include <log/log.hpp>
#include <syscall/sys_fd.hpp>
#include <fs/fs.hpp>

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

    static fs::FileDescriptor* get_fd(int fd) {
        process::Process* process = arch::percpu::current_process();

        if (fd < 0 || (std::size_t)fd >= process->fd_table.size()) {
            return nullptr;
        }
        
        return &process->fd_table[fd];
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
        fs::FileDescriptor* desc = get_fd(fd);

        if (!desc) {
            return -EBADF;
        }

        return desc->inode->ops->read(desc, buffer, count);
    }

    int sys_write(int fd, const void* buffer, std::size_t count) {
        fs::FileDescriptor* desc = get_fd(fd);

        if (!desc) {
            return -EBADF;
        }

        return desc->inode->ops->write(desc, buffer, count);
    }

    int sys_close(int fd) {
        process::Process* process = arch::percpu::current_process();
        fs::FileDescriptor* desc = get_fd(fd);

        if (!desc) {
            return -EBADF;
        }

        int result = desc->inode->ops->close(desc);

        process->fd_table[fd] = {};

        return result;
    }

    int sys_stat(const char* path, fs::Stat* stat) {
        return fs::stat(path, stat);
    }

    int sys_fstat(int fd, fs::Stat* stat) {
        fs::FileDescriptor* desc = get_fd(fd);

        if (!desc) {
            return -EBADF;
        }

        return desc->inode->ops->fstat(desc, stat);
    }

    int sys_lseek(int fd, std::size_t offset, int whence) {
        fs::FileDescriptor* desc = get_fd(fd);

        if (!desc) {
            return -EBADF;
        }

        return desc->inode->ops->lseek(desc, offset, whence);
    }

    int sys_getcwd(char* buffer, std::size_t size) {
        process::Process* proc = arch::percpu::current_process();
        std::size_t len = size-1;//proc->working_dir.length();

        if (len >= size) {
            return -ERANGE;
        }

        //memcpy(buffer, proc->working_dir.c_str(), len);
        buffer[len] = '\0';

        return 0;
    }

    int sys_chdir(const char* buffer, std::size_t size) {
        process::Process* proc = arch::percpu::current_process();
        kstring path{buffer, size};
        fs::Stat stat;

        int stat_res = fs::stat(path, &stat);

        if (stat_res != 0) {
            return stat_res;
        }

        if (stat.type != fs::FileType::DIRECTORY) {
            return -ENOTDIR;
        }

        //proc->working_dir = fs::canonicalize(path);

        return 0;
    }
}
