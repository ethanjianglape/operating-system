#pragma once

#include "containers/klist.hpp"
#include "fs/fs.hpp"
namespace fs::tmpfs {

class TmpFileInode final : public Inode {
public:
    kvector<std::uint8_t> data;
    int open(FileDescriptor* fd, int flags) override;
    int read(FileDescriptor* fd, void* buf, std::size_t count) override;
    int write(FileDescriptor* fd, const void* buf, std::size_t count) override;
    int close(FileDescriptor* fd) override;
    int lseek(FileDescriptor* fd, int offset, int whence) override;
    int stat(Stat* stat) override;
};

class TmpDirectoryInode final : public DirectoryInode {
private:
    klist<Inode*> children;

public:
    Inode* lookup(const char* name) override;
    int readdir(kvector<DirEntry>& entries) override;
    int mkdir(const char* name, int mode) override;
    int create(const char* name, int mode) override;
    int open(FileDescriptor* fd, int flags) override;
    int close(FileDescriptor* fd) override;
    int lseek(FileDescriptor* fd, int offset, int whence) override;
    int stat(Stat* stat) override;
};

class TmpMountPoint final : public MountPoint {
private:
    TmpDirectoryInode* root_inode;
    int next_ino = 1;

public:
    TmpMountPoint();

    DirectoryInode* root() override;
};

class TmpFileSystem final : public FileSystem {
public:
    const char* name() override;
    MountPoint* mount(const char*) override;
};

}
