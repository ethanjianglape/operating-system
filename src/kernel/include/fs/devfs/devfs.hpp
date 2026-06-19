#pragma once

#include <fs/devfs/dev_null.hpp>
#include <fs/devfs/dev_tty.hpp>
#include <fs/fs.hpp>

namespace fs::devfs {

class DevMountPoint;

constexpr int DEV_ROOT_INO = 1;
constexpr int DEV_NULL_INO = 2;
constexpr int DEV_TTY1_INO = 3;
constexpr int DEV_TTY2_INO = 4;

class DevDirectoryInode final : public DirectoryInode {
private:
public:
    DevDirectoryInode(DevMountPoint* mp, int ino);

    Inode* lookup(const char* name) override;
    int readdir(kvector<DirEntry>& entries) override;
    int mkdir(const char* name, int mode) override;
    int create(const char* name, int mode) override;
    int open(FileDescriptor* fd, int flags) override;
    int close(FileDescriptor* fd) override;
    int stat(Stat* stat) override;
};

class DevMountPoint final : public MountPoint {
public:
    DevNullInode* null_inode;
    DevTtyInode* tty1_inode;
    DevTtyInode* tty2_inode;

    DevMountPoint();
};

class DevFileSystem final : public FileSystem {
public:
    const char* name() override;
    MountPoint* mount(const char*) override;
};

}
