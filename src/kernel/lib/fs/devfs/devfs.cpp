#include <fs/devfs/dev_null.hpp>
#include <fs/devfs/dev_tty.hpp>
#include <fs/devfs/devfs.hpp>
#include <fs/fs.hpp>

namespace fs::devfs {

DevDirectoryInode::DevDirectoryInode(DevMountPoint* mp, int ino)
    : DirectoryInode{mp}
{
    this->ino = ino;
}

Inode* DevDirectoryInode::lookup(const char* name)
{
    kstring name_str{name};

    auto dev_mp = static_cast<DevMountPoint*>(mountpoint);

    if (name_str == "null") {
        return dev_mp->null_inode;
    }

    if (name_str == "tty1") {
        return dev_mp->tty1_inode;
    }

    if (name_str == "tty2") {
        return dev_mp->tty2_inode;
    }

    return nullptr;
}

int DevDirectoryInode::readdir(kvector<DirEntry>& entries)
{
    entries.emplace_back("null", FileType::CHAR_DEVICE);
    entries.emplace_back("tty1", FileType::CHAR_DEVICE);
    entries.emplace_back("tty2", FileType::CHAR_DEVICE);

    return entries.size();
}

int DevDirectoryInode::mkdir(const char*, int)
{
    return 0;
}

int DevDirectoryInode::create(const char*, int)
{
    return 0;
}

int DevDirectoryInode::open(FileDescriptor*, int)
{
    return 0;
}

int DevDirectoryInode::close(FileDescriptor*)
{
    return 0;
}

int DevDirectoryInode::stat(Stat*)
{
    return 0;
}

DevMountPoint::DevMountPoint()
{
    root_inode = new DevDirectoryInode{this, DEV_ROOT_INO};
    null_inode = new DevNullInode{this, root_inode, DEV_NULL_INO};
    tty1_inode = new DevTtyInode{this, root_inode, DEV_TTY1_INO};
    tty2_inode = new DevTtyInode{this, root_inode, DEV_TTY2_INO};
}

const char* DevFileSystem::name()
{
    return "devfs";
}

MountPoint* DevFileSystem::mount(const char*)
{
    return new DevMountPoint{};
}

}
