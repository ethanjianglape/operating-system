#include "fs/fs.hpp"
#include <cstdint>
#include <fs/vfs/vfs.hpp>
#include <containers/kvector.hpp>
#include <log/log.hpp>
#include <algo/algo.hpp>

#include <cstddef>

namespace fs::vfs {
    static kvector<MountPoint> mount_points;

    std::intmax_t vfs_read_file(FileDescriptor* desc, void* buffer, std::size_t count);
    std::intmax_t vfs_write_file(FileDescriptor* desc, const void* buffer, std::size_t count);
    std::intmax_t vfs_stat_file(FileDescriptor* desc, Stat* stat);
    std::intmax_t vfs_close_file(FileDescriptor* desc);

    static FileOps vfs_file_ops {
        .read = vfs_read_file,
        .write = vfs_write_file,
        .stat = vfs_stat_file,
        .close = vfs_close_file
    };

    FileOps* get_vfs_file_ops() { return &vfs_file_ops; }

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

    MountPoint* get_mount_point(const kstring& path) {
        log::debug("get mount point for: ", path);
        
        MountPoint* mp_ptr = nullptr;
        
        for (auto& mp : mount_points) {
            if (path.starts_with(mp.root)) {

                mp_ptr = &mp;
            }
        }

        log::debug("match found: ", mp_ptr->root);
        
        return mp_ptr;
    }

    kstring get_relative_path(const kstring& canonical, MountPoint* mp) {
        return canonical.substr(mp->root.size());
    }

    Inode lookup(const char* path, int flags) {
        kstring canonical = canonicalize(path);
        MountPoint* mp = get_mount_point(canonical);

        if (mp == nullptr) {
            return NULL_INODE;
        }

        kstring relative = get_relative_path(canonical, mp);

        log::debug("lookup relative: ", relative);

        return mp->fs->open(relative, flags);
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

    bool is_root_of_mount_point(const kstring& relative, MountPoint* mp) {
        return relative == mp->root;
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

    std::intmax_t vfs_read_file(FileDescriptor* desc, void* buffer, std::size_t count) {
        if (desc->inode.type == FileType::DIRECTORY) {
            return -1;
        }
        
        std::size_t remaining = desc->inode.size - desc->offset;
        std::size_t to_read = (count < remaining) ? count : remaining;

        if (to_read == 0) {
            return EOF;
        }
        
        std::size_t result = desc->inode.fs->read(&desc->inode, buffer, to_read, desc->offset);

        desc->offset += result;

        return result;
    }

    std::intmax_t vfs_write_file(FileDescriptor* desc, const void* buffer, std::size_t count) {
        
    }

    std::intmax_t vfs_stat_file(FileDescriptor* desc, Stat* stat) {
        stat->type = desc->inode.type;
        stat->size = desc->inode.size;
        stat->path = ""; // for now

        return 0;
    }

    std::intmax_t vfs_close_file(FileDescriptor* desc) {
        
    }
}
