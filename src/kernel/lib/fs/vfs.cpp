#include <fs/vfs.hpp>
#include <containers/kvector.hpp>
#include <log/log.hpp>

#include <cstddef>

namespace fs {
    static kvector<FileDescriptor> fd_table;
    static kvector<MountPoint> mount_points;

    int alloc_fd() {
        for (std::size_t i = 0; i < fd_table.size(); i++) {
            if (fd_table[i].inode.metadata == nullptr) {
                return i;
            }
        }

        fd_table.push_back({});

        return fd_table.size() - 1;
    }

    void mount(const kstring& path, FileSystem* fs) {
        for (const auto& mp : mount_points) {
            if (mp.path == path) {
                log::warn("Filesystem already mounted at: ", path);
                return;
            }
        }
        
        mount_points.push_back({
            .path = path,
            .fs = fs
        });
    }
    
    int open(const char* path, int flags) {
        kstring path_str = path;
        FileSystem* fs = nullptr;

        log::debug("fs::open = ", path);
        
        for (auto& mp :  mount_points) {
            log::debug("mp path = ", mp.path);
            if (path_str.starts_with(mp.path)) {
                fs = mp.fs;
            }
        }

        if (!fs) {
            return -1;
        }

        Inode inode = fs->open(path, flags);

        // path not found
        if (inode.metadata == nullptr) {
            return -1;
        }

        int fd = alloc_fd();

        fd_table[fd].inode = inode;
        fd_table[fd].offset = 0;
        fd_table[fd].flags = flags;
        
        return fd;
    }

    int close(int fd) {
        if (fd < 0 || (std::size_t)fd >= fd_table.size()) {
            return -1;
        }

        fd_table[fd].inode = NULL_INODE;
        fd_table[fd].offset = 0;
        fd_table[fd].flags = 0;

        return 1;
    }

    int read(int fd, void* buffer, std::size_t count) {
        if (fd < 0 || (std::size_t)fd >= fd_table.size()) {
            return -1;
        }

        log::debug("read fd = ", fd);

        FileDescriptor& desc = fd_table[fd];

        // Can't read from a dir
        if (desc.inode.type == FileType::Dir) {
            return -1;
        }

        std::size_t remaining = desc.inode.size - desc.offset;
        std::size_t to_read = (count < remaining) ? count : remaining;

        log::debug("rem = ", remaining, " to read = ", to_read);

        if (to_read == 0) {
            return EOF;
        }

        std::size_t result = desc.inode.fs->read(&desc.inode, buffer, to_read, desc.offset);

        desc.offset += result;

        return result;
    }
}
