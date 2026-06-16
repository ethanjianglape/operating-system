#pragma once

#include <fs/devfs/dev_null.hpp>
#include <fs/devfs/dev_tty.hpp>
#include <fs/fs.hpp>

namespace fs::devfs {

class DevMountPoint;

class DevDirectoryInode final : public DirectoryInode {
private:
    DevMountPoint* dev_mp;

public:
    explicit DevDirectoryInode(DevMountPoint* mp);

    InodeClass* lookup(const char* name) override;
    int readdir(kvector<DirEntry>& entries) override;
    int mkdir(const char* name, int mode) override;
    int crete(const char* name, int mode) override;
    int open(FileDescriptor* fd, int flags) override;
    int close(FileDescriptor* fd) override;
    int lseek(FileDescriptor* fd, int offset, int whence) override;
    int stat(Stat* stat) override;
};

class DevMountPoint final : public MountPointClass {
public:
    DevDirectoryInode root_inode;
    DevNullInode null_inode;
    DevTtyInode tty1_inode;
    DevTtyInode tty2_inode;

    DevMountPoint();

    DirectoryInode* root() override;
};

class DevFileSystem final : public FileSystemClass {
public:
    const char* name() override;
    MountPointClass* mount(const char*) override;
};

}
