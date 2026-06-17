#pragma once

#include <cerrno>
#include <containers/kstring.hpp>
#include <containers/kvector.hpp>

#include <cstddef>
#include <cstdint>

namespace fs {
enum class FileType : std::uint8_t {
    NONE = 0,
    REGULAR = 1,
    DIRECTORY = 2,
    CHAR_DEVICE = 3,
};

constexpr int O_RDONLY = 0x01;

constexpr int SEEK_SET = 0;
constexpr int SEEK_CUR = 1;
constexpr int SEEK_END = 2;

struct FileDescriptor;
struct Stat;
struct DirEntry;

class Inode;
class DirectoryInode;
class FileSystem;
class MountPoint;

/**
 * @brief File metadata (for stat() without opening).
 */
struct Stat {
    FileType type;
    std::size_t size;
};

/**
 * @brief Directory listing entry.
 */
struct DirEntry {
    kstring name;
    FileType type;
};

class FileSystem {
public:
    virtual const char* name() = 0;
    virtual MountPoint* mount(const char* dir) = 0;

    FileSystem() {};
    virtual ~FileSystem() = default;
    FileSystem(const FileSystem&) = delete;
    FileSystem(FileSystem&&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;
    FileSystem& operator=(FileSystem&&) = delete;
};

class MountPoint {
public:
    const char* path;
    FileSystem* fs;
    Inode* mounted_on;
    virtual DirectoryInode* root() = 0;
};

class Inode {
public:
    MountPoint* mountpoint;
    Inode* parent;
    std::uint64_t ino;
    std::uint64_t size;
    FileType type;
    kstring name;

    Inode() = default;
    virtual ~Inode() = default;
    Inode(const Inode&) = delete;
    Inode(Inode&&) = delete;
    Inode& operator=(const Inode&) = delete;
    Inode& operator=(Inode&&) = delete;

    virtual Inode* lookup(const char* name) { return nullptr; }
    virtual int open(FileDescriptor* fd, int flags) = 0;
    virtual int read(FileDescriptor* fd, void* buf, std::size_t count) = 0;
    virtual int write(FileDescriptor* fd, const void* buf, std::size_t count) = 0;
    virtual int close(FileDescriptor* fd) = 0;
    virtual int lseek(FileDescriptor* fd, int offset, int whence) = 0;
    virtual int stat(Stat* stat) = 0;
};

class DirectoryInode : public Inode {
public:
    DirectoryInode() = default;
    virtual ~DirectoryInode() = default;

    int read(FileDescriptor*, void*, std::size_t) final override { return -EISDIR; }
    int write(FileDescriptor*, const void*, std::size_t) final override { return -EISDIR; }

    virtual int readdir(kvector<DirEntry>& entries) = 0;
    virtual int mkdir(const char* name, int mode) = 0;
    virtual int create(const char* name, int mode) = 0;
};

/**
 * @brief An open file handle (per-process).
 */
struct FileDescriptor {
    Inode* inode;
    kstring path;
    std::size_t offset;
    int flags;
};

// ============================================================================
// VFS operations - global path-based file access
// ============================================================================

kstring getcwd(const Inode* inode);
void register_mount(const char* path, MountPoint* mp);
FileDescriptor* open(const char* path, int flags);
void mount(const char* path, FileSystem* fs, const char* source);

int stat(const kstring& path, Stat* out);
int readdir(const kstring& path, kvector<DirEntry>& out);
int mkdir(const kstring& path, int mode);
}
