#pragma once

#include "fs/fs.hpp"

namespace fs::tmpfs {

class TmpFileInode final : public Inode {
public:
    kvector<std::uint8_t> data;

    TmpFileInode(kstring name, Inode* parent);

    int open(FileDescriptor* fd, int flags) override;
    int read(FileDescriptor* fd, void* buf, std::size_t count) override;
    int write(FileDescriptor* fd, const void* buf, std::size_t count) override;
    int close(FileDescriptor* fd) override;
    int lseek(FileDescriptor* fd, int offset, int whence) override;
    int stat(Stat* stat) override;
};

class TmpDirectoryInode final : public DirectoryInode {
public:
    TmpDirectoryInode(MountPoint* mp);
    TmpDirectoryInode(kstring name, Inode* parent);

    Inode* lookup(const char* name) override;
    int readdir(kvector<DirEntry>& entries) override;
    int mkdir(const char* name, int mode) override;
    int create(const char* name, int mode) override;
    int open(FileDescriptor* fd, int flags) override;
    int close(FileDescriptor* fd) override;
    int stat(Stat* stat) override;
};

class TmpMountPoint final : public MountPoint {
public:
    int next_ino = 1;
    TmpMountPoint();
};

class TmpFileSystem final : public FileSystem {
public:
    const char* name() override;
    MountPoint* mount(const char*) override;
};

}
