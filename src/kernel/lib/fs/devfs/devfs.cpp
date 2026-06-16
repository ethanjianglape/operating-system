#include <fs/devfs/dev_null.hpp>
#include <fs/devfs/dev_tty.hpp>
#include <fs/devfs/devfs.hpp>
#include <fs/fs.hpp>

namespace fs::devfs {

static DevFileSystem g_devfs{};

DevDirectoryInode::DevDirectoryInode(DevMountPoint* mp)
    : dev_mp{mp}
{
    type = FileType::DIRECTORY;
}

InodeClass* DevDirectoryInode::lookup(const char* name)
{
    kstring name_str{name};

    if (name_str == "null") {
        return &dev_mp->null_inode;
    }

    if (name_str == "tty1") {
        return &dev_mp->tty1_inode;
    }

    if (name_str == "tty2") {
        return &dev_mp->tty2_inode;
    }

    return nullptr;
}

int DevDirectoryInode::readdir(kvector<DirEntry>& entries)
{
    entries.push_back(DirEntry{.name = "null", .type = FileType::CHAR_DEVICE});
    entries.push_back(DirEntry{.name = "tty1", .type = FileType::CHAR_DEVICE});
    entries.push_back(DirEntry{.name = "tty2", .type = FileType::CHAR_DEVICE});

    return entries.size();
}

int DevDirectoryInode::mkdir(const char* name, int mode)
{
    return 0;
}

int DevDirectoryInode::crete(const char* name, int mode)
{
    return 0;
}

int DevDirectoryInode::open(FileDescriptor* fd, int flags)
{
    return 0;
}

int DevDirectoryInode::close(FileDescriptor* fd)
{
    return 0;
}

int DevDirectoryInode::lseek(FileDescriptor* fd, int offset, int whence)
{
    return 0;
}

int DevDirectoryInode::stat(Stat* stat)
{
    return 0;
}

DevMountPoint::DevMountPoint()
    : root_inode{this}
{
    int ino = 1;

    root_inode.ino = ino++;
    root_inode.mountpoint = this;

    null_inode.ino = ino++;
    null_inode.mountpoint = this;
    null_inode.parent = &root_inode;

    tty1_inode.ino = ino++;
    tty1_inode.mountpoint = this;
    tty1_inode.parent = &root_inode;

    tty2_inode.ino = ino++;
    tty2_inode.mountpoint = this;
    tty2_inode.parent = &root_inode;
}

DirectoryInode* DevMountPoint::root() { return &root_inode; }

const char* DevFileSystem::name()
{
    return "devfs";
}

MountPointClass* DevFileSystem::mount(const char*)
{
    return new DevMountPoint{};
}

}
