#include <cstddef>
#include <drivers/acpi/acpi.hpp>
#include <boot/boot.hpp>
#include <boot/limine.h>
#include <arch.hpp>
#include <console/console.hpp>
#include <drivers/framebuffer/framebuffer.hpp>
#include <fmt/fmt.hpp>
#include <log/log.hpp>
#include <memory/pmm.hpp>
#include <fs/initramfs/initramfs.hpp>

#include <cstdint>

[[gnu::used]]
[[gnu::section(".limine_requests_start")]]
static volatile std::uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_START_MARKER;

[[gnu::used]]
[[gnu::section(".limine_requests")]]
static volatile std::uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

[[gnu::used]]
[[gnu::section(".limine_requests")]]
static volatile limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
    .response = nullptr,
    .internal_module_count = 0,
    .internal_modules = nullptr
};

[[gnu::used]]
[[gnu::section(".limine_requests")]]
static volatile limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used]]
[[gnu::section(".limine_requests")]]
static volatile limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used]]
[[gnu::section(".limine_requests")]]
static volatile limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used]]
[[gnu::section(".limine_requests")]]
static volatile limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used]]
[[gnu::section(".limine_requests_end")]]
static volatile std::uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_END_MARKER;

namespace boot {
    void init() {
        log::init_start("Limine Response");

        limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];

        auto* vram = static_cast<std::uint8_t*>(fb->address);
        auto width = fb->width;
        auto height = fb->height;
        auto pitch = fb->pitch;
        auto bpp = fb->bpp;

        auto fb_info = drivers::framebuffer::FrameBufferInfo {
            .width = width,
            .height = height,
            .pitch = pitch,
            .bpp = bpp,
            .vram = vram
        };

        drivers::framebuffer::init(fb_info);

        auto entry_count = memmap_request.response->entry_count;
        limine_memmap_entry** entries = memmap_request.response->entries;

        pmm::init();

        for (std::uint64_t i = 0; i < entry_count; i++) {
            auto base = entries[i]->base;
            auto length = entries[i]->length;
            auto type = entries[i]->type;

            log::info("Limine memmap entry #", i, ": base (", fmt::hex{base}, ") length (", fmt::hex{length}, ") type (", type, ")");

            if (type == LIMINE_MEMMAP_USABLE) {
                pmm::add_free_memory(base, length);
            }
        }

        std::uint64_t hhdm_offset = hhdm_request.response->offset;
        arch::vmm::init(hhdm_offset);

        void* rsdp_address = rsdp_request.response->address;
        drivers::acpi::init(rsdp_address);

        limine_module_response* modules = module_request.response;

        for (std::size_t i = 0; i < modules->module_count; i++) {
            limine_file* mod = modules->modules[i];
            auto addr = static_cast<std::uint8_t*>(mod->address);
            auto size = mod->size;

            log::info("Limine mod path: ", (const char*)mod->path, " addr = ", addr, " size = ", size);

            fs::initramfs::init(addr, size);
        }

        log::init_end("Limine Response");
    }
}
