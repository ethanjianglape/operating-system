#pragma once

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

    // ============================================================================
    // FileOps
    // ============================================================================

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
    };

    // ============================================================================
    // Inode
    // ============================================================================

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

    // ============================================================================
    // FileDescriptor
    // ============================================================================

    /**
     * @brief An open file handle (per-process).
     */
    struct FileDescriptor {
        Inode* inode;
        std::size_t offset;
        int flags;
    };

    // ============================================================================
    // Stat
    // ============================================================================

    /**
     * @brief File metadata (for stat() without opening).
     */
    struct Stat {
        FileType type;
        std::size_t size;
    };

    // ============================================================================
    // DirEntry
    // ============================================================================

    /**
     * @brief Directory listing entry.
     */
    struct DirEntry {
        kstring name;
        FileType type;
    };

    // ============================================================================
    // FileSystem
    // ============================================================================

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
    };

    // ============================================================================
    // MountPoint
    // ============================================================================

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

    void mount(const kstring& path, FileSystem* fs);
    Inode* open(const kstring& path, int flags);
    int stat(const kstring& path, Stat* out);
    int readdir(const kstring& path, kvector<DirEntry>& out);
}
