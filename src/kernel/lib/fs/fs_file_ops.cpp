#include <cerrno>
#include <cstddef>
#include <fs/fs_file_ops.hpp>
#include <fs/fs.hpp>
#include <crt/crt.h>

namespace fs {
    static int fs_file_read(FileDescriptor* fd, void* buf, std::size_t count) {
        if (!fd || !fd->inode || !buf) {
            return -1;
        }

        if (fd->inode->type == FileType::DIRECTORY) {
            return -1;
        }

        if (fd->offset >= fd->inode->size) {
            return 0;  // EOF
        }

        std::size_t remaining = fd->inode->size - fd->offset;
        std::size_t to_read = count < remaining ? count : remaining;

        auto* meta = static_cast<FsFileMeta*>(fd->inode->private_data);
        memcpy(buf, meta->data + fd->offset, to_read);

        fd->offset += to_read;
        return static_cast<int>(to_read);
    }

    static int fs_file_write(FileDescriptor*, const void*, std::size_t) {
        // Read-only for now
        return -1;
    }

    static int fs_file_close(FileDescriptor* fd) {
        if (fd && fd->inode) {
            delete static_cast<FsFileMeta*>(fd->inode->private_data);
            delete fd->inode;
            
            fd->inode = nullptr;
        }
        
        return 0;
    }

    static int fs_file_lseek(FileDescriptor* fd, int offset, int whence) {
        if (!fd || !fd->inode) {
            return -EBADF;
        }

        if (fd->inode->type == FileType::DIRECTORY) {
            return -EISDIR;
        }

        int new_offset;

        switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = fd->offset + offset;
            break;
        case SEEK_END:
            new_offset = fd->inode->size + offset;
            break;
        }

        if (new_offset < 0) {
            return -EINVAL;
        }

        fd->offset = new_offset;

        return new_offset;
    }

    static const FileOps fs_file_ops = {
        .read = fs_file_read,
        .write = fs_file_write,
        .close = fs_file_close,
        .lseek = fs_file_lseek
    };

    const FileOps* get_fs_file_ops() {
        return &fs_file_ops;
    }
}
