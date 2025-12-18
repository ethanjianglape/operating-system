#include "arch/i686/gdt/gdt.hpp"
#include "kernel/drivers/framebuffer/framebuffer.hpp"
#include <kernel/boot/boot.hpp>
#include <kernel/boot/multiboot2.h>
#include <kernel/panic/panic.hpp>
#include <kernel/memory/memory.hpp>
#include <kernel/arch/arch.hpp>

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
            auto tag_type = *reinterpret_cast<std::uint32_t*>(tag);
            auto tag_size = *reinterpret_cast<std::uint32_t*>(tag + 4);

            if (tag_type == MULTIBOOT_TAG_TYPE_END) {
                break;
            }

            if (tag_type == MULTIBOOT_TAG_TYPE_MMAP) {
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
            } else if (tag_type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
                auto* fb = reinterpret_cast<multiboot_tag_framebuffer*>(tag);
                auto* fbc = &fb->common;

                const auto width = fbc->framebuffer_width;
                const auto height = fbc->framebuffer_height;
                const auto bpp = fbc->framebuffer_bpp;
                const auto pitch = fbc->framebuffer_pitch;
                const auto type = fbc->framebuffer_type;
                const auto fb_size = pitch * height;

                if (type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
                    //kernel::panicf("Invalid framebuffer type %d, expected %d (RGB)", type, MULTIBOOT_FRAMEBUFFER_TYPE_RGB);
                }
                
                auto phys_addr = reinterpret_cast<std::uint8_t*>(fbc->framebuffer_addr);
                auto virt_addr = arch::vmm::map_physical_region(phys_addr, fb_size);

                auto fbInfo = kernel::drivers::framebuffer::FrameBufferInfo {
                    .width = width,
                    .height = height,
                    .pitch = pitch,
                    .bpp = bpp,
                    .vram = reinterpret_cast<std::uint8_t*>(virt_addr)
                };

                kernel::drivers::framebuffer::init(fbInfo);
            }

            // advance to the next tag
            tag += (tag_size + 7) & ~7;
        }

        kernel::pmm::init(total_mem, free_mem_addr, free_mem_len);
    }
}
