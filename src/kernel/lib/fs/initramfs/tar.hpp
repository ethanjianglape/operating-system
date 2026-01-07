#pragma once

#include "containers/kstring.hpp"
#include "containers/kvector.hpp"
#include <fs/fs.hpp>

#include <cstddef>
#include <cstdint>

namespace fs::initramfs::tar {
    struct TarHeader {
        char filename[100];   // null-terminated
        char mode[8];         // skip
        char uid[8];          // skip
        char gid[8];          // skip
        char size[12];        // octal ASCII -> parse to integer
        char mtime[12];       // skip
        char checksum[8];     // skip (or validate)
        char typeflag;        // '0' or '\0' = file, '5' = directory
        char linkname[100];   // skip
        char magic[6];        // "ustar" = valid POSIX tar
        char version[2];      // skip
        char uname[32];       // skip
        char gname[32];       // skip
        char devmajor[8];     // skip
        char devminor[8];     // skip
        char prefix[155];     // prepend to filename if present
        char padding[12];
    };

    constexpr char TYPEFLAG_DIR = '5';

    struct TarMeta {
        TarHeader* header;
        std::uint8_t* data;
        std::uintmax_t size_bytes;
        std::uintmax_t num_blocks;
        kstring filename_str;
    };

    void init(std::uint8_t* addr);

    TarMeta* find(const kstring& filename);

    kvector<TarMeta*> list(const kstring& dir);

    std::size_t read(TarMeta* meta, void* buffer, std::size_t count, std::size_t offset);

    kvector<DirEntry> readdir(const kstring& path);
}
