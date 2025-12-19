#include "vmm.hpp"
#include "arch/x86_64/cpu/cpu.hpp"

#include <kernel/log/log.hpp>
#include <kernel/memory/memory.hpp>

#include <cstddef>
#include <cstdint>

using namespace x86_64;

// Raw physical and virtual addresses of the kernel from linker.ld
// These are dynamic as the size of the kernel changes between compiles
extern "C" char kernel_start[];
extern "C" char kernel_end[];

alignas(4096)
static vmm::PageTableEntry* pml4;

static std::uint8_t* kernel_heap;

static bool early_boot = true;

void* access_phys(std::uintptr_t phys){
    if (early_boot) {
        return reinterpret_cast<void*>(phys);
    }

    return vmm::phys_to_virt(phys);
}

void* vmm::map_physical_region(void* physical_addr_ptr, std::size_t bytes) {
    const std::size_t pages = (bytes / PAGE_SIZE) + 1;
    auto phys_addr = reinterpret_cast<std::uintptr_t>(physical_addr_ptr);
    auto page_start = reinterpret_cast<std::uintptr_t>(kernel_heap);

    for (std::size_t page = 0; page < pages; page++) {
        auto virtual_addr = page_start + (page * PAGE_SIZE);

        map_page(virtual_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE | PAGE_CACHE_DISABLE);
        phys_addr += kernel::pmm::FRAME_SIZE;
    }

    kernel_heap += (pages * PAGE_SIZE);

    return reinterpret_cast<void*>(page_start);
}

void make_pte(vmm::PageTableEntry* pte, std::uintptr_t phys, std::uint64_t flags) {
    pte->p = (flags & vmm::PAGE_PRESENT) ? 1 : 0;
    pte->rw = (flags & vmm::PAGE_WRITE) ? 1 : 0;
    pte->us = (flags & vmm::PAGE_USER) ? 1 : 0;
    pte->pcd = (flags & vmm::PAGE_CACHE_DISABLE) ? 1 : 0;
    pte->addr = phys >> 12;
}

void zero_page(void* virt) {
    auto* ptr = static_cast<std::uintptr_t*>(virt);

    for (std::size_t i = 0; i < vmm::NUM_PT_ENTRIES; i++) {
        ptr[i] = 0x0;
    }
}

void vmm::map_page(std::uintptr_t virt, std::uintptr_t phys, std::uint32_t flags) {
    const std::uintptr_t pml4_idx = (virt >> 39) & 0x1FF;
    const std::uintptr_t pdpt_idx = (virt >> 30) & 0x1FF;
    const std::uintptr_t pd_idx   = (virt >> 21) & 0x1FF;
    const std::uintptr_t pt_idx   = (virt >> 12) & 0x1FF;

    //skernel::log::info("Mapping virt (%x) to phys (%x), %d, %d, %d, %d", virt, phys, pml4_idx, pdpt_idx, pd_idx, pt_idx);

    if (!pml4[pml4_idx].p) {
        // Allocate a new 4KiB physical frame for this entry
        auto page_phys = kernel::pmm::alloc_frame<std::uintptr_t>();

        // Create the missing pte entry at this index
        make_pte(&pml4[pml4_idx], page_phys, flags | PAGE_PRESENT | PAGE_WRITE);

        // Zero out the page in the next layer of the page table
        zero_page(access_phys(page_phys));
    }

    auto* pdpt = reinterpret_cast<PageTableEntry*>(access_phys(pml4[pml4_idx].addr << 12));

    if (!pdpt[pdpt_idx].p) {
        // Allocate a new 4KiB physical frame for this entry
        auto page_phys = kernel::pmm::alloc_frame<std::uintptr_t>();

        // Create the missing pte entry at this index
        make_pte(&pdpt[pdpt_idx], page_phys, flags | PAGE_PRESENT | PAGE_WRITE);
                                                                                                                                                    
        // Zero out the page in the next layer of the page table
        zero_page(access_phys(page_phys));
    }

    auto* pd = reinterpret_cast<PageTableEntry*>(access_phys(pdpt[pdpt_idx].addr << 12));

    if (!pd[pd_idx].p) {
        // Allocate a new 4KiB physical frame for this entry
        auto page_phys = kernel::pmm::alloc_frame<std::uintptr_t>();

        // Create the missing pte entry at this index
        make_pte(&pd[pd_idx], page_phys, flags | PAGE_PRESENT | PAGE_WRITE);

        // Zero out the page in the next layer of the page table
        zero_page(access_phys(page_phys));
    }

    auto* pt = reinterpret_cast<PageTableEntry*>(access_phys(pd[pd_idx].addr << 12));

    make_pte(&pt[pt_idx], phys, flags | PAGE_PRESENT | PAGE_WRITE);

    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void* vmm::alloc_kernel_memory(std::size_t bytes) {
    std::size_t pages = (bytes / PAGE_SIZE) + 1;

    return alloc_kernel_pages(pages);
}

void* vmm::alloc_kernel_pages(std::size_t pages) {
    auto page_start = reinterpret_cast<std::uintptr_t>(kernel_heap);

    for (std::size_t page = 0; page < pages; page++) {
        std::uintptr_t phys_addr = kernel::pmm::alloc_frame<std::uintptr_t>();
        std::uintptr_t virt_addr = page_start + (page * PAGE_SIZE);

        map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE);
    }

    kernel_heap += (pages * PAGE_SIZE);

    return reinterpret_cast<void*>(page_start);
}

void vmm::init() {
    kernel::log::init_start("Paging");

    early_boot = true;

    // Identity mapped pml4 for early init
    pml4 = kernel::pmm::alloc_frame<PageTableEntry*>();
    zero_page(pml4);

    std::size_t total_ram = kernel::pmm::get_total_memory();
    const auto virt_base  = reinterpret_cast<std::uintptr_t>(KERNEL_VIRT_BASE);

    kernel::log::info("pml4 addr  = %x", pml4);
    kernel::log::info("System RAM = %d" , total_ram);
    kernel::log::info("virt base  = %x", virt_base);

    // Identity map low memory (first 16MiB)
    for (std::uintptr_t addr = 0; addr < 0x01000000; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITE);
    }

    // Build direct mapping for all physical RAM as KERNEL_VIRT_BASE + phys -> phys
    for (std::uintptr_t phys = 0; phys < total_ram; phys += PAGE_SIZE) {
        map_page(virt_base + phys, phys, PAGE_PRESENT | PAGE_WRITE);
    }

    const auto kphys_start = reinterpret_cast<std::uintptr_t>(kernel_start) - virt_base;
    const auto kphys_end   = reinterpret_cast<std::uintptr_t>(kernel_end) - virt_base;
    const auto kvirt_start = reinterpret_cast<std::uintptr_t>(kernel_start);

    asm volatile("mov %0, %%cr3" : : "r"(pml4));

    early_boot = false;
    pml4 = reinterpret_cast<PageTableEntry*>(phys_to_virt(pml4));

    kernel::log::info("kernel virt base  = %x", KERNEL_VIRT_BASE);
    kernel::log::info("kernel phys start = %x", kphys_start);
    kernel::log::info("kernel phys end   = %x", kphys_end);
    kernel::log::info("kernel virt start = %x", kvirt_start);
    kernel::log::info("pml4 virt addr    = %x", pml4);
    kernel::log::info("pml4 phys addr    = %x", virt_to_phys(pml4));
    kernel::log::info("heap virt start   = %x", kernel_heap);

    kernel::log::init_end("Paging");
}
