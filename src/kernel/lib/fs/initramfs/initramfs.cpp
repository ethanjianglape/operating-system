#include "algo/algo.hpp"
#include "containers/kvector.hpp"
#include "tar.hpp"

#include <cerrno>
#include <cstdint>
#include <fs/fs.hpp>
#include <fs/fs_file_ops.hpp>
#include <fs/initramfs/initramfs.hpp>
#include <log/log.hpp>
#include <memory/memory.hpp>

namespace fs::initramfs {

using namespace tar;

InitramfsDirectoryInode::InitramfsDirectoryInode(InitramfsMountPoint* mp)
    : initramfs_mp{mp}
{
    mountpoint = mp;
    type = FileType::DIRECTORY;
}

InodeClass* InitramfsDirectoryInode::lookup(const char* name)
{
    for (const auto& child : children) {
        if (child.name == name) {
            if (child.dir_inode) {
                return child.dir_inode;
            }

            if (child.file_inode) {
                return child.file_inode;
            }
        }
    }

    return nullptr;
}

int InitramfsDirectoryInode::readdir(kvector<DirEntry>& entries)
{
    for (const auto& child : children) {
        DirEntry entry{};

        entry.name = child.name;
        entry.type = child.dir_inode != nullptr ? FileType::DIRECTORY : FileType::REGULAR;

        entries.push_back(entry);
    }

    return children.size();
}

int InitramfsDirectoryInode::mkdir(const char*, int) { return -EROFS; }

int InitramfsDirectoryInode::crete(const char*, int) { return -EROFS; }

int InitramfsDirectoryInode::open(FileDescriptor*, int) { return 0; }

int InitramfsDirectoryInode::close(FileDescriptor*) { return 0; }

int InitramfsDirectoryInode::lseek(FileDescriptor* fd, int offset, int whence) { return 0; }

int InitramfsDirectoryInode::stat(Stat* stat) { return 0; }

InitramfsFileInode::InitramfsFileInode()
{
    type = FileType::REGULAR;
}

int InitramfsFileInode::open(FileDescriptor* fd, int flags) { return 0; }

int InitramfsFileInode::read(FileDescriptor* fd, void* buf, std::size_t count)
{
    memcpy(buf, tar_data, count);
    return count;
}

int InitramfsFileInode::write(FileDescriptor* fd, const void* buf, std::size_t count) { return 0; }

int InitramfsFileInode::close(FileDescriptor* fd) { return 0; }

int InitramfsFileInode::lseek(FileDescriptor* fd, int offset, int whence) { return 0; }

int InitramfsFileInode::stat(Stat* stat) { return 0; }

bool InitramfsMountPoint::is_empty_header(TarHeader* header)
{
    return header->filename[0] == '\0';
}

void InitramfsMountPoint::insert_inode(const char* path, InitramfsDirectoryInode* dir_inode, InitramfsFileInode* file_inode)
{
    auto* current = root_inode;
    kvector<kstring> tokens = algo::split(path, '/');

    for (std::size_t i = 0; i < tokens.size() - 1; i++) {
        kstring& token = tokens[i];

        InodeClass* existing = current->lookup(token.c_str());

        if (existing) {
            // static_cast here is safe because only fs::initramfs has access
            // to the internal implementation of these data structures, so we trust
            // that `existing` is a valid directory inode
            current = static_cast<InitramfsDirectoryInode*>(existing);
        } else {
            // this token is part of a path that doesn't have an inode yet, maybe because
            // the tar file didn't include a tar entry for it
            auto* dir = new InitramfsDirectoryInode{this};
            dir->ino = next_ino++;
            dir->parent = current;

            InitramfsDirEntry entry{};

            entry.name = token;
            entry.dir_inode = dir;
            entry.file_inode = nullptr;

            current->children.push_back(std::move(entry));
            current = dir;
        }
    }

    const kstring& last = tokens.back();

    InitramfsDirEntry entry{};

    if (dir_inode) {
        dir_inode->parent = current;
    }

    if (file_inode) {
        file_inode->parent = current;
    }

    entry.name = last;
    entry.dir_inode = dir_inode;
    entry.file_inode = file_inode;

    current->children.push_back(std::move(entry));
}

void InitramfsMountPoint::parse_tar_headers()
{
    log::info("Parsing tar headers");

    root_inode = new InitramfsDirectoryInode{this};
    root_inode->mountpoint = this;
    root_inode->ino = next_ino++;
    root_inode->parent = nullptr;

    std::uint8_t* addr = tar_buffer;
    std::uint8_t* end = addr + tar_size;

    log::debugf("tar start = {}, end = {}", fmt::hex{addr}, fmt::hex{end});

    while (addr < end) {
        TarHeader* header = reinterpret_cast<TarHeader*>(addr);

        std::uintmax_t size = fmt::parse_uint((const char*)header->size, 12, fmt::NumberFormat::OCT);
        std::uintmax_t num_blocks = (size + 511) / 512;
        char* filename = header->filename;

        if (is_empty_header(header)) {
            goto next;
        }

        if (filename[0] == '.' && filename[1] == '\0') {
            goto next;
        }

        if (filename[0] == '.' && filename[1] == '/') {
            filename += 2;
        }

        if (filename[0] == '\0') {
            goto next;
        }

        log::debugf("tar header found, size = {}, block = {}, name = '{}'", size, num_blocks, kstring{filename});

        if (header->typeflag == TYPEFLAG_DIR) {
            auto* dir = new InitramfsDirectoryInode{this};

            dir->mountpoint = this;
            dir->ino = next_ino++;

            insert_inode(filename, dir, nullptr);
        } else {
            auto* file = new InitramfsFileInode{};

            file->mountpoint = this;
            file->tar_header = header;
            file->tar_data = addr + 512;
            file->size_bytes = size;
            file->size = size;
            file->num_blocks = num_blocks;
            file->ino = next_ino++;

            insert_inode(filename, nullptr, file);
        }

    next:
        addr += 512 + (num_blocks * 512);
    }
}

DirectoryInode* InitramfsMountPoint::root() { return root_inode; }

const char* InitramfsFileSystem::name() { return "initramfs"; }

MountPointClass* InitramfsFileSystem::mount(const char*) { return nullptr; }

MountPointClass* InitramfsFileSystem::mount_from_mem(std::uint8_t* buffer, std::size_t size)
{
    return new InitramfsMountPoint{buffer, size};
}

static Inode* initramfs_open(FileSystem* self, const kstring& path, int flags);
static int initramfs_stat(FileSystem* self, const kstring& path, Stat* out);
static int initramfs_readdir(FileSystem* self, const kstring& path, kvector<DirEntry>& out);
static int initramfs_mkdir(FileSystem* self, const kstring& path, int mode);

void init(std::uint8_t* addr, std::size_t)
{
    //tar::init(addr);
    //fs::mount("/", &initramfs_fs);
}

static int initramfs_stat(FileSystem*, const kstring& path, Stat* out)
{
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

static int initramfs_readdir(FileSystem*, const kstring& path, kvector<DirEntry>& out)
{
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
