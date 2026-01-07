#include "log/log.hpp"
#include <syscall/fd/syscall_fd.hpp>
#include <fs/vfs/vfs.hpp>
#include <fs/fs.hpp>
#include <containers/kvector.hpp>
#include <containers/kstring.hpp>

#include <cerrno>

namespace syscall::fd {
    static kvector<fs::FileDescriptor> fd_table;

    int alloc_fd() {
        for (std::size_t i = 0; i < fd_table.size(); i++) {
            if (fd_table[i].inode.metadata == nullptr) {
                return i;
            }
        }

        fd_table.push_back({});

        return fd_table.size() - 1;
    }

    int sys_open(const char* path, int flags) {
        log::debug("sys_open: ", path);
        
        fs::Inode inode = fs::vfs::lookup(path, flags);

        if (inode.type == fs::FileType::NOT_FOUND) {
            return -ENOENT;
        }

        int fd = alloc_fd();

        fd_table[fd].inode = inode;
        fd_table[fd].offset = 0;
        fd_table[fd].flags = flags;

        return fd;
    }

    int sys_read(int fd, void* buffer, std::size_t count) {
        fs::FileDescriptor* desc = &fd_table[fd];

        return desc->inode.ops->read(desc, buffer, count);
    }

    int sys_write(int fd, const void* buffer, std::size_t count) {
        fs::FileDescriptor* desc = &fd_table[fd];

        return desc->inode.ops->write(desc, buffer, count);
    }

    int sys_close(int fd) {
        fs::FileDescriptor* desc = &fd_table[fd];

        int result = desc->inode.ops->close(desc);

        fd_table[fd] = {};

        return result;
    }

    int sys_fstat(int fd, fs::Stat* stat) {
        fs::FileDescriptor* desc = &fd_table[fd];

        return desc->inode.ops->stat(desc, stat);
    }
}
