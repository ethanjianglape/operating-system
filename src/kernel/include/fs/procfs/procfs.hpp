#pragma once

#include <fs/fs.hpp>
#include <fs/procfs/proc_self.hpp>

namespace fs::procfs {

constexpr int PROCFS_ROOT_INODE = 1;

class ProcMountPoint final : public MountPoint {
public:
    ProcSelfInode* self_inode;

    ProcMountPoint();
};

class ProcDirectoryInode final : public DirectoryInode {
public:
    ProcDirectoryInode(ProcMountPoint* mp, int ino);

    Inode* lookup(const char* name) override;
    int readdir(kvector<DirEntry>& entries) override;
    int mkdir(const char* name, int mode) override;
    int create(const char* name, int mode) override;
    int open(FileDescriptor* fd, int flags) override;
    int close(FileDescriptor* fd) override;
    int stat(Stat* stat) override;
};

class ProcFileSystem final : public FileSystem {
public:
    const char* name() override;
    MountPoint* mount(const char*) override;
};

}
