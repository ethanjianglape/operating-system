#pragma once

#include <fs/fs.hpp>

namespace fs::procfs {

class ProcSelfInode : public ReadOnlyInode {
public:
    ProcSelfInode(MountPoint* mp, Inode* parent, int ino);

    int read(FileDescriptor* fd, void* buf, std::size_t count) final override;
};

}
