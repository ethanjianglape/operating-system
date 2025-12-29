#include "tar.hpp"
#include "containers/kstring.hpp"
#include "fmt/fmt.hpp"
#include "log/log.hpp"

#include <containers/kvector.hpp>
#include <cstddef>
#include <cstdint>

namespace fs::initramfs::tar {
    kvector<TarMeta> metas;

    bool is_empty_header(TarHeader* header) {
        return header->filename[0] == '\0';
    }

    void parse_headers(std::uint8_t* addr) {
        TarHeader* header = reinterpret_cast<TarHeader*>(addr);

        if (header == nullptr || is_empty_header(header)) {
            return;
        }

        
        kstring filename;

        if (header->prefix[0] != '\0') {
            filename += header->prefix + 2;
            filename += '/';
            filename += header->filename;
        } else {
            filename += header->filename + 2;
        }

        if (filename.back() == '/') {
            filename.pop_back();
        }

        std::uintmax_t size = fmt::parse_uint((const char*)header->size, 12, fmt::NumberFormat::OCT);
        std::uintmax_t num_blocks = (size + 511) / 512;
        std::uint8_t* data = size > 0 ? addr + 512 : nullptr;

        metas.push_back(TarMeta{
            .header = header,
            .data = data,
            .size_bytes = size,
            .num_blocks = num_blocks,
            .filename_str = filename
        });

        parse_headers(addr + 512 + (num_blocks * 512));
    }

    void init(std::uint8_t* addr) {
        parse_headers(addr);

        for (auto&& header : metas) {
            log::info("TAR header: filename = ", header.filename_str,
                      ", size = ", header.size_bytes,
                      ", #blocks = ", header.num_blocks);
        }
    }

    TarMeta* find(const kstring& filename) {
        for (std::size_t i = 0; i < metas.size(); i++) {
            if (metas[i].filename_str == filename) {
                return &metas[i];
            }
        }
        
        return nullptr;
    }

    std::size_t read(TarMeta* meta, void* buffer, std::size_t count, std::size_t offset) {
        std::uint8_t* data = meta->data + offset;

        log::debug("tar read from ", fmt::hex{data}, " to = ", fmt::hex{buffer}, " count = ", count, " offset = ", offset);

        memcpy(buffer, data, count);

        return count;
    }

    kvector<TarMeta*> list(const kstring& dir) {
        kvector<TarMeta*> entries;

        bool is_root = dir.empty();

        for (auto& meta : metas) {
            const kstring& filename = meta.filename_str;
            
            if (!filename.starts_with(dir) || dir == filename) {
                continue;
            }

            bool is_dir = meta.header->typeflag == '5';
            int dir_count = 0;

            for (std::size_t i = dir.length(); i < filename.length(); i++) {
                if (filename[i] == '/') {
                    dir_count++;
                }
            }

            // we are in root, no sub dirs should be listed
            if (is_root && dir_count > 0) {
                continue;
            }

            // This is a sub-sub dir
            if (is_dir && dir_count > 1) {
                continue;
            }

            // This is a file in a sub-dir
            if (!is_dir && dir_count > 1) {
                continue;
            }

            // Else this is a dir/file in dir
            entries.push_back(&meta);
        }

        return entries;
    }
}
