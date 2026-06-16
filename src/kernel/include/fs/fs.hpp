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

struct Inode;
struct FileDescriptor;
struct FileSystem;
struct Stat;
struct DirEntry;

class InodeClass;
class DirectoryInode;
class FileSystemClass;
class MountPointClass;

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

class FileSystemClass {
public:
    virtual const char* name() = 0;
    virtual MountPointClass* mount(const char* dir) = 0;

    FileSystemClass() {};
    virtual ~FileSystemClass() = default;
    FileSystemClass(const FileSystemClass&) = delete;
    FileSystemClass(FileSystemClass&&) = delete;
    FileSystemClass& operator=(const FileSystemClass&) = delete;
    FileSystemClass& operator=(FileSystemClass&&) = delete;
};

class MountPointClass {
public:
    const char* path;
    FileSystemClass* fs;
    InodeClass* mounted_on;
    virtual DirectoryInode* root() = 0;
};

class InodeClass {
public:
    MountPointClass* mountpoint;
    InodeClass* parent;
    std::uint64_t ino;
    std::uint64_t size;
    FileType type;
    kstring name;

    InodeClass() = default;
    virtual ~InodeClass() = default;
    InodeClass(const InodeClass&) = delete;
    InodeClass(InodeClass&&) = delete;
    InodeClass& operator=(const InodeClass&) = delete;
    InodeClass& operator=(InodeClass&&) = delete;

    virtual InodeClass* lookup(const char* name) { return nullptr; }
    virtual int open(FileDescriptor* fd, int flags) = 0;
    virtual int read(FileDescriptor* fd, void* buf, std::size_t count) = 0;
    virtual int write(FileDescriptor* fd, const void* buf, std::size_t count) = 0;
    virtual int close(FileDescriptor* fd) = 0;
    virtual int lseek(FileDescriptor* fd, int offset, int whence) = 0;
    virtual int stat(Stat* stat) = 0;
};

class DirectoryInode : public InodeClass {
public:
    DirectoryInode() = default;
    virtual ~DirectoryInode() = default;

    int read(FileDescriptor*, void*, std::size_t) final override { return -EISDIR; }
    int write(FileDescriptor*, const void*, std::size_t) final override { return -EISDIR; }

    virtual int readdir(kvector<DirEntry>& entries) = 0;
    virtual int mkdir(const char* name, int mode) = 0;
    virtual int crete(const char* name, int mode) = 0;
};

/**
 * @brief Operations on an open file (fd-level).
 *
 * Each file type (regular, char device, etc.) provides its own implementation.
 * The implementation decides how to handle offset (use it, ignore it, etc.)
 */
struct FileOps {
    int (*read)(FileDescriptor* fd, void* buf, std::size_t count);
    int (*write)(FileDescriptor* fd, const void* buf, std::size_t count);
    int (*close)(FileDescriptor* fd);
    int (*lseek)(FileDescriptor* fd, int offset, int whence);
    int (*fstat)(FileDescriptor* fd, Stat* stat);
};

/**
 * @brief A file or directory in the filesystem.
 *
 * Created by FileSystem::open(), freed by FileOps::close().
 */
struct Inode {
    FileType type;
    std::size_t size;
    const FileOps* ops;
    void* private_data;
};

/**
 * @brief An open file handle (per-process).
 */
struct FileDescriptor {
    InodeClass* inode;
    kstring path;
    std::size_t offset;
    int flags;
};

/**
 * @brief A mounted filesystem (path-based operations).
 *
 * Handles path resolution and inode creation. Does NOT handle read/write.
 */
struct FileSystem {
    const char* name;
    void* private_data;

    Inode* (*open)(FileSystem* self, const kstring& path, int flags);
    int (*stat)(FileSystem* self, const kstring& path, Stat* out);
    int (*readdir)(FileSystem* self, const kstring& path, kvector<DirEntry>& out);
    int (*mkdir)(FileSystem* self, const kstring& path, int mode);
};

/**
 * @brief A filesystem mounted at a path.
 */
struct MountPoint {
    kstring root;
    FileSystem* filesystem;
};

// ============================================================================
// VFS operations - global path-based file access
// ============================================================================

kstring canonicalize(const kstring& path);
kstring getcwd(const InodeClass* inode);
void register_mount(const char* path, MountPointClass* mp);
FileDescriptor* open(const char* path, int flags);
void mount(const char* path, FileSystemClass* fs, const char* source);

int stat(const kstring& path, Stat* out);
int readdir(const kstring& path, kvector<DirEntry>& out);
int mkdir(const kstring& path, int mode);
}
