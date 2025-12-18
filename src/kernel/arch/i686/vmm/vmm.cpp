#include "vmm.hpp"

#include "kernel/log/log.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/panic/panic.hpp"

#include <cstddef>
#include <cstdint>

using namespace i686;

[[gnu::aligned(4096)]]
static vmm::page_directory_entry pdt[vmm::NUM_PDT_ENTRIES];

// The kernel is in higher-half mode now (0xC0000000, 3GB) but the very low end of the
// address space (0-32 MB) cannot safely be given to userspace, this memory range is
// used by many hardware and boot processes that are not safe to expose. So userspace
// will instead start at 32MB.
[[gnu::aligned(4096)]]
static vmm::page_table_entry kernel_low_pte[vmm::KERNEL_LOW_PT_SIZE][vmm::NUM_PT_ENTRIES];

// This is the higher-half address space for the kernel to use, at pdt[768-832] starts at 0xC0000000 and covers 256MB
[[gnu::aligned(4096)]]
static vmm::page_table_entry kernel_high_pte[vmm::KERNEL_HIGH_PT_SIZE][vmm::NUM_PT_ENTRIES];

// Until we have dynamic memory allocation, just statically give userspace 32MB of address space
// starting at 32MB (0-32MB reserved for hardware)
[[gnu::aligned(4096)]]
static vmm::page_table_entry user_pte[vmm::USER_PT_SIZE][vmm::NUM_PT_ENTRIES];

// The APIC uses MMIO at a very specific memory address, so this needs to be mapped
[[gnu::aligned(4096)]]
static vmm::page_table_entry apic_pte[vmm::NUM_PT_ENTRIES];

// Heap space used by the kernel for things like kmalloc()
static std::uint8_t* kernel_heap = reinterpret_cast<std::uint8_t*>(vmm::KERNEL_HEAP_VIRT_BASE);

void init_kernel_pde(vmm::page_directory_entry* pde, vmm::page_table_entry* pte) {
    pde->p = 1;   // Present
    pde->rw = 1;  // Writable
    pde->us = 0;  // ring0 only
    pde->pwt = 0; // write-through disabled
    pde->pcd = 0; // Cache enabled
    pde->a = 0;   // Accessed (managed by hardware)
    pde->ign = 0; // Ignore in 32bit mode
    pde->ps = 0;  // Page size (always 0 for 4KB)
    pde->avl = 0; // Available

    // Our page tables are static variables created within our c++ file, so they have a virtual address
    // that is in our higher-half address space, but the PDT must have physical addresses
    pde->addr = vmm::virt_to_phys(pte) >> 12;
}

void init_pdt() {
    // Kernel low address space (0-32MB)
    for (std::uint32_t i = 0; i < vmm::KERNEL_LOW_PT_SIZE; i++) {
        auto* pde = &pdt[vmm::KERNEL_LOW_PDE_START + i];
        auto* pte = kernel_low_pte[i];

        init_kernel_pde(pde, pte);
    }

    // Kernel high address space (3GB+)
    for (std::uint32_t i = 0; i < vmm::KERNEL_HIGH_PT_SIZE; i++) {
        auto* pde = &pdt[vmm::KERNEL_HIGH_PDE_START + i];
        auto* pte = kernel_high_pte[i];

        init_kernel_pde(pde, pte);
    }

    // Userspace (32-64MB)
    for (std::uint32_t i = 0; i < vmm::USER_PT_SIZE; i++) {
        auto* pde = &pdt[vmm::USER_PDE_START + i];
        auto* pte = user_pte[i];

        pde->p = 1;
        pde->rw = 1;
        pde->us = 1; // User accessible
        pde->pwt = 0;
        pde->pcd = 0;
        pde->a = 0;
        pde->ign = 0;
        pde->ps = 0;
        pde->avl = 0;
        pde->addr = vmm::virt_to_phys(pte) >> 12;
    }

    // The APIC uses memory mapped IO calls at address (0xFEE00000)
    // Until we have all of pdt mapped, this needs to be explicitly set
    // 0xFEE00000 >> 22 = 0x3FB (1019) = PDE index
    // 0xFEE00000 >> 12 = 0xFEE00 & 0x3FB = 0x000 = PTE index
    auto* apic_pde = &pdt[vmm::APIC_PDE_START];

    apic_pde->p = 1;
    apic_pde->rw = 1;
    apic_pde->us = 0;
    apic_pde->pwt = 1;
    apic_pde->pcd = 1;
    apic_pde->a = 0;
    apic_pde->ign = 0;
    apic_pde->ps = 0;
    apic_pde->avl = 0;
    apic_pde->addr = vmm::virt_to_phys(apic_pte) >> 12;
}

void init_pte() {
    // Kernel low address space (0-32MB)
    for (std::uint32_t table = 0; table < vmm::KERNEL_LOW_PT_SIZE; table++) {
        for (std::uint32_t i = 0; i < vmm::NUM_PT_ENTRIES; i++) {
            std::uint32_t page_frame = (table * vmm::NUM_PT_ENTRIES) + i;
            
            kernel_low_pte[table][i].p = 1;
            kernel_low_pte[table][i].rw = 1;
            kernel_low_pte[table][i].us = 0;
            kernel_low_pte[table][i].addr = page_frame;
        }
    }

    // Kernel high address space (3GB+)
    for (std::uint32_t table = 0; table < vmm::KERNEL_HIGH_PT_SIZE; table++) {
        for (std::uint32_t i = 0; i < vmm::NUM_PT_ENTRIES; i++) {
            std::uint32_t page_frame = (table * vmm::NUM_PT_ENTRIES) + i;
            
            kernel_high_pte[table][i].p = 1;
            kernel_high_pte[table][i].rw = 1;
            kernel_high_pte[table][i].us = 0;
            kernel_high_pte[table][i].addr = page_frame;
        }
    }

    // Userspace (32-64MB)
    for (std::uint32_t table = 0; table < vmm::USER_PT_SIZE; table++) {
        for (std::uint32_t i = 0; i < vmm::NUM_PT_ENTRIES; i++) {
            std::uint32_t base_frame = vmm::KERNEL_LOW_PT_SIZE * vmm::NUM_PT_ENTRIES;
            std::uint32_t page_frame = base_frame + (table * vmm::NUM_PT_ENTRIES) + i;
            
            user_pte[table][i].p = 1;
            user_pte[table][i].rw = 1;
            user_pte[table][i].us = 1; // User accessible
            user_pte[table][i].addr = page_frame;
        }
    }

    // APIC address space
    for (std::uint32_t i = 0; i < vmm::NUM_PT_ENTRIES; i++) {
        apic_pte[i].p = 1;
        apic_pte[i].rw = 1;
        apic_pte[i].us = 0;
        apic_pte[i].pcd = 1;            // Disable cache for MMIO
        apic_pte[i].addr = 0xFEE00 + i; // physical address 0xFEE00000 >> 12
    }
}

void enable_paging() {
    std::uint32_t cr0;
    std::uintptr_t pdt_addr = vmm::virt_to_phys(pdt);
    
    asm volatile("mov %0, %%cr3" : : "r"(pdt_addr) : "memory");
    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    cr0 |= 0x80000000; // set bit 31 to enable paging

    asm volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

void* vmm::map_physical_region(void* physical_addr, std::size_t bytes) {
    const std::size_t pages = (bytes / PAGE_SIZE) + 1;
    auto physical_addr_ptr = reinterpret_cast<std::uint8_t*>(physical_addr);
    void* page_start = kernel_heap;

    for (std::size_t page = 0; page < pages; page++) {
        void* virtual_addr = kernel_heap + (page * PAGE_SIZE);

        map_page(virtual_addr, physical_addr_ptr, PAGE_PRESENT | PAGE_WRITE | PAGE_CACHE_DISABLE);
        physical_addr_ptr += kernel::pmm::FRAME_SIZE;
    }

    kernel_heap += (pages * PAGE_SIZE);

    return page_start;
}

void vmm::map_page(void* virtual_addr, void* physical_addr, std::uint32_t flags) {
    const auto pde_index = reinterpret_cast<std::uint32_t>(virtual_addr) >> 22;
    const auto pte_index = reinterpret_cast<std::uint32_t>(virtual_addr) >> 12 & 0x3FF;

    page_table_entry* pte = nullptr;

    if (pde_index >= KERNEL_LOW_PDE_START && pde_index <= KERNEL_LOW_PDE_END) {
        pte = &kernel_low_pte[pde_index - KERNEL_LOW_PDE_START][pte_index];
    } else if (pde_index >= KERNEL_HIGH_PDE_START && pde_index <= KERNEL_HIGH_PDE_END) {
        pte = &kernel_high_pte[pde_index - KERNEL_HIGH_PDE_START][pte_index];
    }

    if (pte == nullptr) {
        kernel::panicf("Attempt to map invalid virtual address = %u", virtual_addr);
    }

    pte->p = (flags & PAGE_PRESENT) ? 1 : 0;
    pte->rw = (flags & PAGE_WRITE) ? 1 : 0;
    pte->us = (flags & PAGE_USER) ? 1 : 0;
    pte->pcd = (flags & PAGE_CACHE_DISABLE) ? 1 : 0;
    pte->addr = reinterpret_cast<std::uint32_t>(physical_addr) >> 12;

    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

void* vmm::alloc_kernel_memory(std::size_t bytes) {
    std::size_t pages = (bytes / PAGE_SIZE) + 1;

    return alloc_kernel_pages(pages);
}

void* vmm::alloc_kernel_pages(std::size_t num_pages) {
    const auto heap = reinterpret_cast<std::uint32_t>(kernel_heap);

    if (heap + (num_pages * PAGE_SIZE) > KERNEL_HEAP_VIRT_END) {
        kernel::panicf("Out of virtual memory, cannot allocate %d pages", num_pages);
    }

    void* page_start = kernel_heap;

    for (std::size_t page = 0; page < num_pages; page++) {
        void* phys_addr = kernel::pmm::alloc_frame();
        void* virt_addr = kernel_heap + (page * PAGE_SIZE);

        if (phys_addr == nullptr) {
            kernel::panic("Physical Memory Manager returned NULL, system is out of memory");
        }

        map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE);
    }

    kernel_heap += (num_pages * PAGE_SIZE);

    return page_start;
}

void vmm::init() {
    kernel::log::init_start("Paging");
    
    init_pdt();
    init_pte();
    
    enable_paging();

    kernel::log::init_end("Paging");
}
