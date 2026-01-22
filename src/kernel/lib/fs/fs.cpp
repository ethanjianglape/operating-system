#include <fs/fs.hpp>
#include <log/log.hpp>
#include <algo/algo.hpp>

namespace fs {
    static kvector<MountPoint> mount_points;

    static kstring canonicalize(const kstring& path) {
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

    static MountPoint* find_mount(const kstring& path) {
        MountPoint* best = nullptr;

        for (auto& mp : mount_points) {
            if (path.starts_with(mp.root)) {
                if (!best || mp.root.size() > best->root.size()) {
                    best = &mp;
                }
            }
        }

        return best;
    }

    static kstring strip_mount_prefix(const kstring& path, MountPoint* mp) {
        return path.substr(mp->root.size());
    }

    void mount(const kstring& path, FileSystem* fs) {
        for (const auto& mp : mount_points) {
            if (mp.root == path) {
                log::warn("Filesystem already mounted at: ", path);
                return;
            }
        }

        log::debug("fs: mounting ", fs->name, " at ", path);
        mount_points.push_back({
            .root = path,
            .filesystem = fs
        });
    }

    Inode* open(const kstring& path, int flags) {
        kstring canonical = canonicalize(path);
        MountPoint* mp = find_mount(canonical);

        if (!mp) {
            log::debug("fs::open: no mount for ", canonical);
            return nullptr;
        }

        kstring relative = strip_mount_prefix(canonical, mp);

        return mp->filesystem->open(mp->filesystem, relative, flags);
    }

    int stat(const kstring& path, Stat* out) {
        kstring canonical = canonicalize(path);
        MountPoint* mp = find_mount(canonical);

        if (!mp) {
            return -1;
        }

        if (!mp->filesystem->stat) {
            return -1;
        }

        kstring relative = strip_mount_prefix(canonical, mp);
        return mp->filesystem->stat(mp->filesystem, relative, out);
    }

    int readdir(const kstring& path, kvector<DirEntry>& out) {
        kstring canonical = canonicalize(path);
        MountPoint* mp = find_mount(canonical);

        if (!mp) {
            return -1;
        }

        if (!mp->filesystem->readdir) {
            return -1;
        }

        kstring relative = strip_mount_prefix(canonical, mp);

        return mp->filesystem->readdir(mp->filesystem, relative, out);
    }
}
