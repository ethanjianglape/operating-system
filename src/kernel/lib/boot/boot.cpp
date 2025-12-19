#include "kernel/drivers/framebuffer/framebuffer.hpp"
#include <kernel/boot/boot.hpp>
#include <kernel/boot/multiboot2.h>
#include <kernel/memory/memory.hpp>
#include <kernel/arch/arch.hpp>
#include <kernel/log/log.hpp>
#include <kernel/boot/limine.h>

#include <cstdint>
#include <cstddef>

[[gnu::used]]
[[gnu::section(".limine_requests")]]
volatile limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used]]
[[gnu::section(".limine_requests")]]
volatile limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used]]
[[gnu::section(".limine_requests")]]
volatile limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

namespace kernel::boot {
    void init() {
        kernel::log::info("framebuffer response = %x", framebuffer_request.response);
        kernel::log::info("memmap response      = %x", memmap_request.response);
        kernel::log::info("hhdm response        = %x", hhdm_request.response);

        limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];

        auto* vram = static_cast<std::uint32_t*>(fb->address);
        auto width = fb->width;
        auto height = fb->height;
        auto pitch = fb->pitch;
        auto bpp = fb->bpp;

        kernel::log::info("Framebuffer: %dx%dx%d (pitch=%d)", width, height, bpp, pitch);
        kernel::log::info("Framebuffer vram: %x", vram);

        auto entry_count = memmap_request.response->entry_count;
        limine_memmap_entry** entries = memmap_request.response->entries;
        
        constexpr std::size_t min_free_mem_len = 0x00200000;

        std::size_t total_mem = 0;
        std::size_t free_mem_addr = 0;
        std::size_t free_mem_len = 0;

        for (std::uint64_t i = 0; i < entry_count; i++) {
            auto base = entries[i]->base;
            auto length = entries[i]->length;
            auto type = entries[i]->type;

            if (type == LIMINE_MEMMAP_USABLE && length > min_free_mem_len) {
                free_mem_addr = base;
                free_mem_len = length;
                total_mem += length;
            }
        }
    }



    
    void init(std::uint32_t mb_magic, std::uint32_t mbi_addr) {
        if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
            //kernel::panicf("Multiboot2 incorrect magic value: %x expected %x", mb_magic, MULTIBOOT2_BOOTLOADER_MAGIC);
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

                    kernel::log::info("Multiboot MMAP Tag addr=%x, len=%x, type=%d", addr, len, type);

                    if (type == MULTIBOOT_MEMORY_AVAILABLE && len > min_free_mem_len) {
                        free_mem_addr = addr;
                        free_mem_len = len;
                        total_mem += len;
                    }
                    
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

                if (type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
                    //kernel::panicf("Invalid framebuffer type %d, expected %d (RGB)", type, MULTIBOOT_FRAMEBUFFER_TYPE_RGB);
                }

                auto phys_addr = reinterpret_cast<std::uint8_t*>(fbc->framebuffer_addr);

                auto fbInfo = kernel::drivers::framebuffer::FrameBufferInfo {
                    .width = width,
                    .height = height,
                    .pitch = pitch,
                    .bpp = bpp,
                    .physical_addr = phys_addr
                };

                kernel::drivers::framebuffer::config(fbInfo);
            }

            // advance to the next tag
            tag += (tag_size + 7) & ~7;
        }

        kernel::pmm::init(total_mem, free_mem_addr, free_mem_len);
    }
}
