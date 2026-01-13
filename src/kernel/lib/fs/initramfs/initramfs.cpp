#include "containers/kstring.hpp"
#include "containers/kvector.hpp"
#include "log/log.hpp"
#include "tar.hpp"

#include <cstddef>
#include <cstdint>
#include <fs/initramfs/initramfs.hpp>
#include <fs/fs.hpp>
#include <fs/vfs/vfs.hpp>

namespace fs::initramfs {
    static std::uint8_t* fs_addr = nullptr;
    static std::size_t fs_size;

    static FileSystem initramfs_fs = {
        .name = "initramfs",
        .read = read,
        .open = open,
        .readdir = readdir
    };
    
    void init(std::uint8_t* addr, std::size_t size) {
        fs_addr = addr;
        fs_size = size;

        tar::init(addr);
        vfs::mount("/", &initramfs_fs);
    }

    std::intmax_t read(Inode* inode, void* buffer, std::size_t count, std::size_t offset) {
        if (inode == nullptr || buffer == nullptr) {
            return -1;
        }

        tar::TarMeta* meta = reinterpret_cast<tar::TarMeta*>(inode->metadata);

        return tar::read(meta, buffer, count, offset);
    }

    Inode open(const kstring& path, int flags) {
        log::debug("initramfs::open = ", path);
        
        tar::TarMeta* meta = tar::find(path);

        if (meta == nullptr) {
            return vfs::NULL_INODE;
        }

        FileType type = meta->header->typeflag == tar::TYPEFLAG_DIR ? FileType::DIRECTORY : FileType::FILE;
        std::size_t size = meta->size_bytes;

        log::debug("size = ", size);

        return {
            .type = type,
            .size = size,
            .metadata = meta,
            .fs = &initramfs_fs,
            .ops = vfs::get_vfs_file_ops()
        };
    }

    kvector<DirEntry> readdir(const kstring& path) {
        kvector<DirEntry> entries;
        kvector<tar::TarMeta*> metas = tar::list(path);

        for (tar::TarMeta* meta : metas) {
            entries.push_back(DirEntry{
                .type = meta->header->typeflag == tar::TYPEFLAG_DIR ? FileType::DIRECTORY : FileType::FILE,
                .size = meta->size_bytes,
                .name = meta->filename_str
            });
        }
        
        return entries;
    }
}
