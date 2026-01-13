#include <boot/boot.hpp>
#include <boot/limine.h>

#include <arch.hpp>
#include <drivers/acpi/acpi.hpp>
#include <drivers/framebuffer/framebuffer.hpp>
#include <fmt/fmt.hpp>
#include <fs/initramfs/initramfs.hpp>
#include <fs/devfs/devfs.hpp>
#include <log/log.hpp>
#include <memory/pmm.hpp>
#include <panic/panic.hpp>

#include <cstddef>
#include <cstdint>

// ============================================================================
// Limine Request Markers and Structures
// ============================================================================
//
// These structures are placed in special linker sections that Limine scans
// at boot. Limine populates the 'response' pointers before jumping to _start.
//
// The [[gnu::used]] attribute prevents the compiler from optimizing these away,
// and [[gnu::section]] places them in the correct linker sections.
// ============================================================================

[[gnu::used, gnu::section(".limine_requests_start")]]
static volatile std::uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

[[gnu::used, gnu::section(".limine_requests")]]
static volatile std::uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

[[gnu::used, gnu::section(".limine_requests")]]
static volatile limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
    .response = nullptr,
    .internal_module_count = 0,
    .internal_modules = nullptr
};

[[gnu::used, gnu::section(".limine_requests_end")]]
static volatile std::uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

const char* memmap_type_to_string(std::uint64_t type) {
    switch (type) {
        case LIMINE_MEMMAP_USABLE:                  return "Usable";
        case LIMINE_MEMMAP_RESERVED:                return "Reserved";
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:        return "ACPI Reclaimable";
        case LIMINE_MEMMAP_ACPI_NVS:                return "ACPI NVS";
        case LIMINE_MEMMAP_BAD_MEMORY:              return "Bad Memory";
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:  return "Bootloader Reclaimable";
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:  return "Kernel and Modules";
        case LIMINE_MEMMAP_FRAMEBUFFER:             return "Framebuffer";
        default:                                    return "Unknown";
    }
}

void validate_limine_responses() {
    if (framebuffer_request.response == nullptr) {
        panic("Limine framebuffer response is null");
    }
    
    if (memmap_request.response == nullptr) {
        panic("Limine memory map response is null");
    }
    
    if (hhdm_request.response == nullptr) {
        panic("Limine HHDM response is null");
    }
    
    if (rsdp_request.response == nullptr) {
        panic("Limine RSDP response is null");
    }

    log::info("All required Limine responses present");
}

void init_framebuffer() {
    limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];

    if (fb == nullptr) {
        panic("No framebuffer available from Limine");
    }

    auto fb_info = drivers::framebuffer::FrameBufferInfo {
        .width = fb->width,
        .height = fb->height,
        .pitch = fb->pitch,
        .bpp = fb->bpp,
        .vram = static_cast<std::uint8_t*>(fb->address)
    };

    drivers::framebuffer::init(fb_info);
}

void init_memory() {
    auto entry_count = memmap_request.response->entry_count;
    limine_memmap_entry** entries = memmap_request.response->entries;

    log::info("Memory map has ", entry_count, " entries:");

    // Initialize PMM before adding free regions
    pmm::init();

    std::uint64_t total_usable = 0;

    for (std::uint64_t i = 0; i < entry_count; i++) {
        auto base = entries[i]->base;
        auto length = entries[i]->length;
        auto type = entries[i]->type;

        log::info("  [", i, "] ", fmt::hex{base}, " - ", fmt::hex{base + length},
                  " (", fmt::hex{length}, ") ", memmap_type_to_string(type));

        if (type == LIMINE_MEMMAP_USABLE) {
            pmm::add_free_memory(base, length);
            total_usable += length;
        }
    }

    log::info("Total usable memory: ", total_usable / 1024 / 1024, " MiB");

    // Initialize VMM with the Higher Half Direct Map offset
    std::uint64_t hhdm_offset = hhdm_request.response->offset;
    arch::vmm::init(hhdm_offset);
}

void init_acpi() {
    void* rsdp_address = rsdp_request.response->address;

    log::info("RSDP address: ", rsdp_address);

    drivers::acpi::init(rsdp_address);
}

void init_modules() {
    limine_module_response* modules = module_request.response;

    if (modules == nullptr || modules->module_count == 0) {
        log::warn("No Limine modules loaded - initramfs will be unavailable");
        return;
    }

    log::info("Loading ", modules->module_count, " module(s):");

    for (std::size_t i = 0; i < modules->module_count; i++) {
        limine_file* mod = modules->modules[i];
        auto addr = static_cast<std::uint8_t*>(mod->address);
        auto size = mod->size;
        const char* path = mod->path;

        log::info("  [", i, "] ", path, " (", size, " bytes)");

        // TODO: Check module type/extension before assuming initramfs
        // For now, we assume all modules are TAR archives for initramfs
        fs::initramfs::init(addr, size);
    }

    fs::devfs::init();
}

namespace boot {
    void init() {
        log::init_start("Limine Boot");

        validate_limine_responses();
        init_framebuffer();
        init_memory();
        init_acpi();
        init_modules();

        log::init_end("Limine Boot");
    }
}
