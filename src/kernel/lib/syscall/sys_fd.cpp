#include "containers/kstring.hpp"
#include <arch.hpp>
#include <cstddef>
#include <process/process.hpp>
#include <log/log.hpp>
#include <syscall/sys_fd.hpp>
#include <fs/fs.hpp>
#include <console/console.hpp>

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
        process->fd_table[fd].path = fs::canonicalize(path);

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

    int sys_writev(int fd, const linux::iovec* iov, int iovcnt) {
        fs::FileDescriptor* desc = get_fd(fd);

        log::debug("sys_writev fd=", fd, " iovcnt=", iovcnt);

        if (!desc) {
            log::debug("sys_writev fd = null");
            return -EBADF;
        }

        if (iovcnt < 0) {
            log::debug("sys_writev iovcnt < 0");
            return -EINVAL;
        }

        int total = 0;

        for (int i = 0; i < iovcnt; i++) {
            if (iov[i].iov_len == 0) continue;

            int written = desc->inode->ops->write(desc, iov[i].iov_base, iov[i].iov_len);

            if (written < 0) {
                return total > 0 ? total : written;
            }

            total += written;
        }

        return total;
    }

    int sys_ioctl(int fd, unsigned long request, void* arg) {
        fs::FileDescriptor* desc = get_fd(fd);

        if (!desc) {
            return -EBADF;
        }

        if (request == linux::TIOCGWINSZ && desc->inode->type == fs::FileType::CHAR_DEVICE) {
            auto* ws = reinterpret_cast<linux::winsize*>(arg);
            ws->ws_row = console::get_screen_rows();
            ws->ws_col = console::get_screen_cols();
            ws->ws_xpixel = 0;
            ws->ws_ypixel = 0;

            return 0;
        }

        return -ENOTTY;
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

    long sys_getcwd(char* buffer, std::size_t size) {
        process::Process* proc = arch::percpu::current_process();
        std::size_t len = proc->working_dir.length();

        if (len + 1 > size) {
            return -ERANGE;
        }

        memcpy(buffer, proc->working_dir.c_str(), len);
        buffer[len] = '\0';

        return reinterpret_cast<long>(buffer);
    }

    int sys_chdir(const char* buffer) {
        process::Process* proc = arch::percpu::current_process();
        kstring path{buffer};
        fs::Stat stat;

        int stat_res = fs::stat(path, &stat);

        if (stat_res != 0) {
            return stat_res;
        }

        if (stat.type != fs::FileType::DIRECTORY) {
            return -ENOTDIR;
        }

        proc->working_dir = fs::canonicalize(path);

        return 0;
    }

    int sys_fchdir(int fd) {
        process::Process* proc = arch::percpu::current_process();
        fs::FileDescriptor* desc = get_fd(fd);

        if (!desc) {
            return -EBADF;
        }

        if (desc->inode->type != fs::FileType::DIRECTORY) {
            return -ENOTDIR;
        }

        proc->working_dir = desc->path;

        return 0;
    }
}
