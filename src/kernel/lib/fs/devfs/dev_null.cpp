#include <fs/devfs/dev_null.hpp>
#include <fs/fs.hpp>

namespace fs::devfs {
DevNullInode::DevNullInode()
{
    type = FileType::CHAR_DEVICE;
}

int DevNullInode::open(FileDescriptor*, int)
{
    return 0;
}

int DevNullInode::read(FileDescriptor*, void*, std::size_t)
{
    return 0; // EOF
}

int DevNullInode::write(FileDescriptor*, const void*, std::size_t count)
{
    return count; // Discard, report success
}

int DevNullInode::close(FileDescriptor*)
{
    return 0;
}

int DevNullInode::lseek(FileDescriptor*, int, int)
{
    return 0;
}

int DevNullInode::stat(Stat* stat)
{
    stat->size = 0;
    stat->type = FileType::CHAR_DEVICE;

    return 0;
}
}
