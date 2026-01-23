/**
 * @file acpica_glue.cpp
 * @brief C/C++ glue layer implementation for ACPICA OS Services Layer
 *
 * Implements the extern "C" functions declared in acpica_glue.h,
 * bridging ACPICA's C code to the kernel's C++ APIs.
 */

#include "acpica_glue.h"
#include <arch.hpp>
#include <memory/memory.hpp>

extern "C" {

void* acpica_glue_alloc(unsigned long size) {
    return kmalloc(size);
}

void acpica_glue_free(void* ptr) {
    if (ptr) {
        kfree(ptr);
    }
}

void* acpica_glue_map_physical(unsigned long phys_addr, unsigned long size) {
    (void)size; // TODO: handle multi-page mappings if needed
    // Map with write and cache-disable flags for ACPI tables/registers
    constexpr std::uint32_t PAGE_WRITE = 0x02;
    constexpr std::uint32_t PAGE_CACHE_DISABLE = 0x10;
    return reinterpret_cast<void*>(
        arch::vmm::map_hddm_page(phys_addr, PAGE_WRITE | PAGE_CACHE_DISABLE)
    );
}

unsigned long acpica_glue_get_hhdm_offset(void) {
    return arch::vmm::get_hhdm_offset();
}

unsigned char acpica_glue_inb(unsigned short port) {
    return arch::cpu::inb(port);
}

void acpica_glue_outb(unsigned short port, unsigned char value) {
    arch::cpu::outb(port, value);
}

unsigned short acpica_glue_inw(unsigned short port) {
    return arch::cpu::inw(port);
}

void acpica_glue_outw(unsigned short port, unsigned short value) {
    arch::cpu::outw(port, value);
}

unsigned int acpica_glue_inl(unsigned short port) {
    return arch::cpu::inl(port);
}

void acpica_glue_outl(unsigned short port, unsigned int value) {
    arch::cpu::outl(port, value);
}

} // extern "C"
