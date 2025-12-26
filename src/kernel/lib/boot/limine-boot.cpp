#include <kernel/drivers/acpi/acpi.hpp>
#include <kernel/boot/boot.hpp>
#include <kernel/boot/limine.h>
#include <kernel/arch/arch.hpp>
#include <kernel/console/console.hpp>
#include <kernel/drivers/framebuffer/framebuffer.hpp>
#include <fmt/fmt.hpp>
#include <kernel/log/log.hpp>
#include <kernel/memory/pmm.hpp>

#include <cstdint>

[[gnu::used]]
[[gnu::section(".limine_requests_start")]]
static volatile std::uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_START_MARKER;

[[gnu::used]]
[[gnu::section(".limine_requests")]]
static volatile std::uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

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
        log::info("Switching to framebuffer logging");

        log::info("TESTING", 1,2,3,4,5);

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

        log::init_end("Limine Response");
    }
}
