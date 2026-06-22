#include "arch/x64/percpu/percpu.hpp"
#include "containers/kstring.hpp"
#include "linux/dirent.hpp"
#include <arch.hpp>
#include <console/console.hpp>
#include <cstddef>
#include <fs/fs.hpp>
#include <log/log.hpp>
#include <process/process.hpp>
#include <syscall/sys_fd.hpp>

#include <cerrno>

namespace syscall {
static int alloc_fd(process::Process* process)
{
    for (std::size_t i = 0; i < process->fd_table.size(); i++) {
        if (process->fd_table[i]->inode == nullptr) {
            return i;
        }
    }

    process->fd_table.push_back({});

    return process->fd_table.size() - 1;
}

static fs::FileDescriptor* get_fd(int fd)
{
    process::Process* process = arch::percpu::current_process();

    if (fd < 0 || process == nullptr || (std::size_t)fd >= process->fd_table.size()) {
        return nullptr;
    }

    return process->fd_table[fd];
}

int sys_open(const char* path, int flags)
{
    process::Process* process = arch::percpu::current_process();
    fs::FileDescriptor* desc = fs::open(path, flags);

    if (!desc) {
        return -ENOENT;
    }

    int fd = alloc_fd(process);
    process->fd_table[fd] = desc;
    return fd;
}

int sys_read(int fd, void* buffer, std::size_t count)
{
    fs::FileDescriptor* desc = get_fd(fd);

    if (!desc) {
        return -EBADF;
    }

    return desc->inode->read(desc, buffer, count);
}

int sys_write(int fd, const void* buffer, std::size_t count)
{
    fs::FileDescriptor* desc = get_fd(fd);

    if (!desc) {
        return -EBADF;
    }

    return desc->inode->write(desc, buffer, count);
}

int sys_readv(int fd, const linux::iovec* iov, int iovcnt)
{
    fs::FileDescriptor* desc = get_fd(fd);

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
        if (iov[i].iov_len == 0) {
            continue;
        }

        int read = desc->inode->read(desc, iov[i].iov_base, iov[i].iov_len);

        if (read < 0) {
            return total > 0 ? total : read;
        }

        total += read;
    }

    return total;
}

int sys_writev(int fd, const linux::iovec* iov, int iovcnt)
{
    fs::FileDescriptor* desc = get_fd(fd);

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
        if (iov[i].iov_len == 0) {
            continue;
        }

        int written = desc->inode->write(desc, iov[i].iov_base, iov[i].iov_len);

        if (written < 0) {
            return total > 0 ? total : written;
        }

        total += written;
    }

    return total;
}

int sys_ioctl(int fd, unsigned long request, void* arg)
{
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

int sys_close(int fd)
{
    process::Process* process = arch::percpu::current_process();
    fs::FileDescriptor* desc = get_fd(fd);

    if (!desc) {
        return -EBADF;
    }

    int result = desc->inode->close(desc);
    process->fd_table[fd] = nullptr;
    return result;
}

int sys_stat(const char* path, fs::Stat* stat)
{
    return fs::stat(path, stat);
}

int sys_fstat(int fd, fs::Stat* stat)
{
    fs::FileDescriptor* desc = get_fd(fd);

    if (!desc) {
        return -EBADF;
    }

    return desc->inode->stat(stat);
}

int sys_lseek(int fd, std::size_t offset, int whence)
{
    fs::FileDescriptor* desc = get_fd(fd);

    if (!desc) {
        return -EBADF;
    }

    return desc->inode->lseek(desc, offset, whence);
}

long sys_getcwd(char* buffer, std::size_t size)
{
    process::Process* proc = arch::percpu::current_process();
    kstring cwd = fs::getcwd(proc->cwd_inode);

    if (cwd.length() + 1 > size) {
        return -ERANGE;
    }

    memcpy(buffer, cwd.c_str(), cwd.size());
    buffer[cwd.size()] = '\0';

    return reinterpret_cast<long>(buffer);
}

int sys_chdir(const char* path)
{
    auto* proc = arch::percpu::current_process();
    auto* fd = fs::open(path, fs::O_RDONLY);

    if (fd == nullptr) {
        return -EBADF;
    }

    if (fd->inode->type != fs::FileType::DIRECTORY) {
        return -ENOTDIR;
    }

    proc->cwd_inode = fd->inode;

    return 0;
}

int sys_fchdir(int fd)
{
    process::Process* proc = arch::percpu::current_process();
    fs::FileDescriptor* desc = get_fd(fd);

    if (!desc) {
        return -EBADF;
    }

    if (desc->inode->type != fs::FileType::DIRECTORY) {
        return -ENOTDIR;
    }

    proc->cwd_inode = desc->inode;

    return 0;
}

int sys_mkdir(const char* path, int mode)
{
    log::debug("sys_mkdir: ", path);
    return fs::mkdir(path, mode);
}

int sys_fcntl(int, unsigned int, unsigned long)
{
    log::warn("SYS_FCNTL is not implemented yet");
    return 1;
}

int sys_getdents64(int fd, void* buffer, unsigned int count)
{
    fs::FileDescriptor* desc = get_fd(fd);

    if (!desc) {
        return -EBADF;
    }

    kvector<fs::DirEntry> entries;

    fs::readdir(desc->path, entries);

    if (desc->offset >= entries.size()) {
        return 0;
    }

    const int max_entries = count / sizeof(linux::linux_dirent64);

    int inode = 1;
    int entries_returned = 0;

    auto* dirent = reinterpret_cast<linux::linux_dirent64*>(buffer);

    while (desc->offset < entries.size() && entries_returned < max_entries) {
        const auto& entry = entries[desc->offset];

        dirent->d_ino = inode++;
        dirent->d_off = 0;
        dirent->d_reclen = sizeof(linux::linux_dirent64);
        dirent->d_type = static_cast<std::uint8_t>(entry.type);

        strncpy(dirent->d_name, entry.name.c_str(), sizeof(dirent->d_name));

        dirent++;
        entries_returned++;
        desc->offset++;
    }

    return sizeof(linux::linux_dirent64) * entries_returned;
}
}
