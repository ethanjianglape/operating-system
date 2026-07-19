#include <cerrno>
#include <containers/kvector.hpp>
#include <fs/fs.hpp>
#include <fs/fs_file_ops.hpp>
#include <fs/tmpfs/tmpfs.hpp>
#include <log/log.hpp>

#include <cstddef>
#include <cstdint>

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
}

TmpFileInode::TmpFileInode(kstring name, Inode* parent)
    : Inode{parent->mountpoint}
{
    this->name = name;
    this->type = FileType::REGULAR;
    this->parent = parent;
    this->ino = static_cast<TmpMountPoint*>(parent->mountpoint)->next_ino++;
    this->size = 0;
    this->data = {};
}

int TmpFileInode::open(FileDescriptor*, int) { return 0; }

int TmpFileInode::close(FileDescriptor*) { return 0; }

int TmpFileInode::write(FileDescriptor* fd, const void* voidp, std::size_t count)
{
    const auto* buff = static_cast<const std::uint8_t*>(voidp);

    data.resize(fd->offset + count);

    for (std::size_t i = 0; i < count; i++) {
        data[fd->offset++] = *(buff + i);
    }

    size = data.size();

    log::debugf("tmpfs::write size={}", size);

    return count;
}

int TmpFileInode::read(FileDescriptor* fd, void* voidp, std::size_t count)
{
    auto* buff = static_cast<std::uint8_t*>(voidp);

    log::debugf("tmpfs::read() offset={}, size={}, count={}", fd->offset, data.size(), count);

    if (fd->offset >= data.size()) {
        return 0;
    }

    const std::size_t available = data.size() - fd->offset;
    const std::size_t to_read = count < available ? count : available;

    for (std::size_t i = 0; i < to_read; i++) {
        *(buff + i) = data[fd->offset++];
    }

    return to_read;
}

int TmpFileInode::lseek(FileDescriptor* fd, int offset, int whence)
{
    const auto fd_offset = static_cast<std::intmax_t>(fd->offset);
    const auto ino_size = static_cast<std::intmax_t>(size);

    switch (whence) {
    case SEEK_SET:
        if (offset < 0) {
            return -EINVAL;
        }

        fd->offset = offset;
        break;
    case SEEK_CUR:
        if (fd_offset + offset < 0) {
            return -EINVAL;
        }

        fd->offset += offset;
        break;
    case SEEK_END:
        if (ino_size + offset < 0) {
            return -EINVAL;
        }

        fd->offset = size + offset;
        break;
    default:
        log::warn("tmpfs::lseek() invalid whence=", whence);
        return -EINVAL;
    }

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
    this->name = "";
    this->parent = nullptr;
}

TmpDirectoryInode::TmpDirectoryInode(kstring name, Inode* parent)
    : DirectoryInode{parent->mountpoint}
{
    this->name = name;
    this->parent = parent;
    this->ino = static_cast<TmpMountPoint*>(parent->mountpoint)->next_ino++;
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

        entries.emplace_back(inode->name, inode->type);
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

    parent->children.push_back(new TmpDirectoryInode{name, parent});

    return 0;
}

int TmpDirectoryInode::create(const char* name, int)
{
    TmpDirectoryInode* parent = this;
    Inode* existing = parent->lookup(name);

    if (existing) {
        return -1;
    }

    parent->children.push_back(new TmpFileInode{name, parent});

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
