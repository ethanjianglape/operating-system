#include "containers/kvector.hpp"
#include "log/log.hpp"
#include <cstddef>
#include <fs/fs.hpp>
#include <fs/fs_file_ops.hpp>
#include <fs/tmpfs/tmpfs.hpp>

namespace fs::tmpfs {

const char* TmpFileSystem::name() { return "tmpfs"; }

MountPoint* TmpFileSystem::mount(const char*)
{
    return new TmpMountPoint{};
}

TmpMountPoint::TmpMountPoint()
{
    root_inode = new TmpDirectoryInode{this};
    root_inode->ino = next_ino++;
    root_inode->parent = nullptr;
}

TmpFileInode::TmpFileInode(MountPoint* mp)
    : Inode{mp}
{
}

int TmpFileInode::open(FileDescriptor*, int) { return 0; }
int TmpFileInode::close(FileDescriptor*) { return 0; }

int TmpFileInode::write(FileDescriptor*, const void*, std::size_t)
{
    return 0;
}

int TmpFileInode::read(FileDescriptor*, void*, std::size_t)
{
    return 0;
}

int TmpFileInode::lseek(FileDescriptor*, int, int)
{
    return 0;
}

int TmpFileInode::stat(Stat* stat)
{
    stat->size = size;
    stat->type = type;
    return 0;
}

TmpDirectoryInode::TmpDirectoryInode(MountPoint* mp)
    : DirectoryInode{mp}
{
}

Inode* TmpDirectoryInode::lookup(const char* name)
{
    for (std::size_t i = 0; i < children.size(); i++) {
        Inode* inode = children[i];

        if (inode->name == name) {
            return inode;
        }
    }

    return nullptr;
}

int TmpDirectoryInode::readdir(kvector<DirEntry>& entries)
{
    for (std::size_t i = 0; i < children.size(); i++) {
        Inode* inode = children[i];
        DirEntry entry{};

        entry.name = inode->name;
        entry.type = inode->type;

        entries.push_back(entry);
    }

    return children.size();
}

int TmpDirectoryInode::mkdir(const char* name, int)
{
    TmpDirectoryInode* parent = this;
    Inode* existing = parent->lookup(name);

    log::debugf("tmp mkdir name={}", name);

    if (existing) {
        return -1;
    }

    auto* dir = new TmpDirectoryInode{parent->mountpoint};

    dir->type = FileType::DIRECTORY;
    // dir->ino = parent->mountpoint->ne
    dir->parent = parent;
    dir->name = name;

    parent->children.push_back(dir);

    return 0;
}

int TmpDirectoryInode::create(const char* name, int)
{
    TmpDirectoryInode* parent = this;
    Inode* existing = parent->lookup(name);

    if (existing) {
        return -1;
    }

    auto* file = new TmpFileInode{parent->mountpoint};

    file->type = FileType::REGULAR;
    // dir->ino = parent->mountpoint->ne
    file->parent = parent;
    file->name = name;
    file->size = 0;
    file->data = {};

    parent->children.push_back(file);

    return 0;
}

int TmpDirectoryInode::open(FileDescriptor*, int) { return 0; }
int TmpDirectoryInode::close(FileDescriptor*) { return 0; }
int TmpDirectoryInode::stat(Stat* stat)
{
    stat->size = size;
    stat->type = type;

    return 0;
}

}
