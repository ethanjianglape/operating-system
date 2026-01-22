#include "tar.hpp"

#include <fs/initramfs/initramfs.hpp>
#include <fs/fs.hpp>
#include <fs/fs_file_ops.hpp>
#include <memory/memory.hpp>
#include <log/log.hpp>

namespace fs::initramfs {
    static Inode* initramfs_open(FileSystem* self, const kstring& path, int flags);
    static int initramfs_stat(FileSystem* self, const kstring& path, Stat* out);
    static int initramfs_readdir(FileSystem* self, const kstring& path, kvector<DirEntry>& out);

    static FileSystem initramfs_fs = {
        .name = "initramfs",
        .private_data = nullptr,
        .open = initramfs_open,
        .stat = initramfs_stat,
        .readdir = initramfs_readdir,
    };

    void init(std::uint8_t* addr, std::size_t) {
        tar::init(addr);
        fs::mount("/", &initramfs_fs);
    }

    static Inode* initramfs_open(FileSystem*, const kstring& path, int) {
        tar::TarMeta* meta = tar::find(path);
        if (!meta) {
            return nullptr;
        }

        FileType type = meta->header->typeflag == tar::TYPEFLAG_DIR
            ? FileType::DIRECTORY
            : FileType::REGULAR;

        auto* inode = new Inode{};
        inode->type = type;
        inode->size = meta->size_bytes;
        inode->ops = get_fs_file_ops();

        auto* file_meta = new FsFileMeta{};
        file_meta->data = meta->data;
        file_meta->size = meta->size_bytes;
        inode->private_data = file_meta;

        return inode;
    }

    static int initramfs_stat(FileSystem*, const kstring& path, Stat* out) {
        tar::TarMeta* meta = tar::find(path);
        if (!meta) {
            return -1;
        }

        out->type = meta->header->typeflag == tar::TYPEFLAG_DIR
            ? FileType::DIRECTORY
            : FileType::REGULAR;
        out->size = meta->size_bytes;
        
        return 0;
    }

    static int initramfs_readdir(FileSystem*, const kstring& path, kvector<DirEntry>& out) {
        tar::TarMeta* entry = tar::find(path);

        if (entry == nullptr) {
            return -1;
        }
        
        kvector<tar::TarMeta*> metas = tar::list(path);

        for (tar::TarMeta* meta : metas) {
            const kstring& fullpath = meta->filename_str;
            std::size_t prefix_len = path.empty() ? 0 : path.length() + 1;
            
            kstring basename = fullpath.substr(prefix_len);

            out.push_back(DirEntry{
                .name = basename,
                .type = meta->header->typeflag == tar::TYPEFLAG_DIR
                    ? FileType::DIRECTORY
                    : FileType::REGULAR,
            });
        }

        return 0;
    }
}
