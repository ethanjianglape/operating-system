#include <kernel/boot/boot.hpp>
#include <kernel/boot/multiboot2.h>
#include <kernel/panic/panic.hpp>
#include <kernel/memory/memory.hpp>

#include <cstdint>
#include <cstddef>

namespace kernel::boot {
    void init(std::uint32_t mb_magic, std::uint32_t mbi_addr) {
        if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
            kernel::panicf("Multiboot2 incorrect magic value: %x expected %x", mb_magic, MULTIBOOT2_BOOTLOADER_MAGIC);
        }

        std::uint32_t* mbi_ptr = reinterpret_cast<std::uint32_t*>(mbi_addr);
        std::uint32_t mbi_size = *mbi_ptr;

        std::uint8_t* tag = reinterpret_cast<std::uint8_t*>(mbi_addr + 8);
        std::uint8_t* end = reinterpret_cast<std::uint8_t*>(mbi_addr + mbi_size);

        std::size_t total_mem = 0;
        std::size_t free_mem_addr = 0;
        std::size_t free_mem_len = 0;

        // Do not consider memory <2MiB as being available
        constexpr std::size_t min_free_mem_len = 0x00200000;

        while (tag < end) {
            auto type = *reinterpret_cast<std::uint32_t*>(tag);
            auto size = *reinterpret_cast<std::uint32_t*>(tag + 4);

            if (type == MULTIBOOT_TAG_TYPE_END) {
                break;
            }

            if (type == MULTIBOOT_TAG_TYPE_MMAP) {
                auto* mmap = reinterpret_cast<multiboot_tag_mmap*>(tag);
                auto entry_count = (mmap->size - sizeof(multiboot_tag_mmap)) / mmap->entry_size;
                auto* entry = mmap->entries;

                for (std::size_t i = 0; i < entry_count; i++) {
                    auto addr = entry->addr;
                    auto len = entry->len;
                    auto type = entry->type;

                    if (type == MULTIBOOT_MEMORY_AVAILABLE && len > min_free_mem_len) {
                        free_mem_addr = addr;
                        free_mem_len = len;
                    }

                    total_mem += len;
                    
                    auto* entry_addr = reinterpret_cast<std::uint8_t*>(entry);
                    entry = reinterpret_cast<multiboot_mmap_entry*>(entry_addr + mmap->entry_size);
                }
            }

            // advance to the next tag
            tag += (size + 7) & ~7;
        }

        kernel::pmm::init(total_mem, free_mem_addr, free_mem_len);
    }
}
