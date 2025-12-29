#include <fs/vfs.hpp>
#include <containers/kvector.hpp>
#include <log/log.hpp>
#include <algo/algo.hpp>

#include <cstddef>

namespace fs {
    static kvector<FileDescriptor> fd_table;
    static kvector<MountPoint> mount_points;

    Stat stat_not_found(const kstring& path) {
        return Stat {
            .type = FileType::NOT_FOUND,
            .size = 0,
            .path = path
        };
    }

    Stat stat_directory(const kstring& path) {
        return Stat {
            .type = FileType::DIRECTORY,
            .size = 0,
            .path = path
        };
    }

    kstring canonicalize(const char* path) {
        kvector<kstring> canonical;
        kvector<kstring> parts = algo::split(path, '/');

        for (const auto& part : parts) {
            if (part.empty() || part == ".") {
                continue;
            }

            if (part == "..") {
                if (!canonical.empty()) {
                    canonical.pop_back();                    
                }
            } else {
                canonical.push_back(part);
            }
        }

        return "/" + algo::join(canonical, '/');
    }

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
            if (mp.root == path) {
                log::warn("Filesystem already mounted at: ", path);
                return;
            }
        }
        
        mount_points.push_back({
            .root = path,
            .fs = fs
        });
    }

    MountPoint* get_mount_point(const kstring& path) {
        MountPoint* mp_ptr = nullptr;
        
        for (auto& mp : mount_points) {
            if (path.starts_with(mp.root)) {
                mp_ptr = &mp;
            }
        }
        
        return mp_ptr;
    }

    kstring get_relative_path(const kstring& canonical, MountPoint* mp) {
        return canonical.substr(mp->root.size());
    }

    bool is_root_of_mount_point(const kstring& relative, MountPoint* mp) {
        return relative == mp->root;
    }

    Stat stat(const char* path) {
        kstring canonical = canonicalize(path);
        MountPoint* mp = get_mount_point(canonical);

        if (mp == nullptr) {
            return stat_not_found(canonical);
        }

        FileSystem* fs = mp->fs;
        kstring relative = get_relative_path(canonical, mp);

        if (is_root_of_mount_point(relative, mp)) {
            return stat_directory(canonical);
        }

        if (relative[0] == '/') {
            relative.erase(0);
        }

        Inode inode = fs->open(relative.c_str(), O_RDONLY);

        return {
            .type = inode.type,
            .size = inode.size,
            .path = canonical
        };
    }

    kvector<DirEntry> readdir(const char* path) {
        kstring canonical = canonicalize(path);
        MountPoint* mp = get_mount_point(canonical);

        if (mp == nullptr) {
            return {};
        }

        FileSystem* fs = mp->fs;
        kstring relative = get_relative_path(canonical, mp);

        log::debug("readdir canonical = ", canonical, " relative = ", relative);

        if (relative[0] == '/') {
            relative.erase(0);
        }

        return fs->readdir(relative);
    }
    
    int open(const char* path, int flags) {
        kstring path_str = path;
        kstring root;
        FileSystem* fs = nullptr;

        log::debug("fs::open = ", path);
        
        for (auto& mp :  mount_points) {
            log::debug("mp path = ", mp.root);
            if (path_str.starts_with(mp.root)) {
                root = mp.root;
                fs = mp.fs;
            }
        }

        if (!fs) {
            return -1;
        }

        kstring relative = path_str.substr(root.size());

        if (relative[0] == '/') {
            relative.erase(0);
        }

        log::debug("root = ", root.c_str(), " len = ", root.size());
        log::debug("relative path = ", relative.c_str());

        Inode inode = fs->open(relative.c_str(), flags);

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
        if (desc.inode.type == FileType::DIRECTORY) {
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
