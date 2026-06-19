#pragma once

#include <fs/fs.hpp>

namespace fs::devfs {
class DevNullInode final : public Inode {
public:
    DevNullInode(MountPoint* mp, Inode* parent, int ino);

    int open(FileDescriptor*, int) override;
    int read(FileDescriptor*, void*, std::size_t) override;
    int write(FileDescriptor*, const void*, std::size_t count) override;
    int close(FileDescriptor*) override;
    int lseek(FileDescriptor*, int, int) override;
    int stat(Stat* stat) override;
};
}
