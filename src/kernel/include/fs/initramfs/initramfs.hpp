#pragma once

#include "containers/kstring.hpp"
#include "fs/fs.hpp"
#include "lib/fs/initramfs/tar.hpp"
#include <cstddef>
#include <cstdint>

namespace fs::initramfs {

class InitramfsMountPoint;
class InitramfsFileInode;
class InitramfsDirectoryInode;

struct InitramfsDirEntry final {
    kstring name;
    InitramfsDirectoryInode* dir_inode;
    InitramfsFileInode* file_inode;
};

class InitramfsDirectoryInode final : public DirectoryInode {
public:
    InitramfsMountPoint* initramfs_mp;
    kvector<InitramfsDirEntry> children;

    explicit InitramfsDirectoryInode(InitramfsMountPoint* mp);

    InodeClass* lookup(const char* name) override;
    int readdir(kvector<DirEntry>& entries) override;
    int mkdir(const char* name, int mode) override;
    int crete(const char* name, int mode) override;
    int open(FileDescriptor* fd, int flags) override;
    int close(FileDescriptor* fd) override;
    int lseek(FileDescriptor* fd, int offset, int whence) override;
    int stat(Stat* stat) override;
};

class InitramfsFileInode final : public InodeClass {
public:
    tar::TarHeader* tar_header;
    std::uint8_t* tar_data;
    std::size_t size_bytes;
    std::size_t num_blocks;
    kstring filename_str;

    InitramfsFileInode();

    int open(FileDescriptor* fd, int flags) override;
    int read(FileDescriptor* fd, void* buf, std::size_t count) override;
    int write(FileDescriptor* fd, const void* buf, std::size_t count) override;
    int close(FileDescriptor* fd) override;
    int lseek(FileDescriptor* fd, int offset, int whence) override;
    int stat(Stat* stat) override;
};

class InitramfsMountPoint final : public MountPointClass {
private:
    InitramfsDirectoryInode* root_inode;
    std::uint8_t* tar_buffer;
    std::size_t tar_size;
    int next_ino = 1;

    bool is_empty_header(tar::TarHeader* header);
    void parse_tar_headers();
    void insert_inode(const char* path, InitramfsDirectoryInode* dir_inode, InitramfsFileInode* file_inode);

public:
    InitramfsMountPoint(std::uint8_t* buffer, std::size_t size)
        : tar_buffer{buffer}
        , tar_size{size}
    {
        parse_tar_headers();
    }

    DirectoryInode* root() override;
};

class InitramfsFileSystem final : public FileSystemClass {
private:
public:
    const char* name() override;
    MountPointClass* mount(const char* dir) override;
    MountPointClass* mount_from_mem(std::uint8_t* buffer, std::size_t size);
};

}
