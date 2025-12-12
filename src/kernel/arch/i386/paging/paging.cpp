#include "paging.hpp"
#include "kernel/log/log.hpp"

#include <cstdint>

using namespace i386;

[[gnu::aligned(4096)]]
static paging::page_directory_entry pdt[paging::NUM_PDT_ENTRIES];

// The kernel is in higher-half mode now (0xC0000000, 3GB) but the very low end of the
// address space (0-32 MB) cannot safely be given to userspace, this memory range is
// used by many hardware and boot processes that are not safe to expose. So userspace
// will instead start at 32MB.
[[gnu::aligned(4096)]]
static paging::page_table_entry kernel_low_pte[paging::KERNEL_LOW_PT_SIZE][paging::NUM_PT_ENTRIES];

// This is the higher-half address space for the kernel to use, at pdt[768-832] and covers 256MB
[[gnu::aligned(4096)]]
static paging::page_table_entry kernel_high_pte[paging::KERNEL_HIGH_PT_SIZE][paging::NUM_PT_ENTRIES];

// Until we have dynamic memory allocation, just statically give userspace 32MB of address space
// starting at 32MB (0-32MB reserved for hardware)
[[gnu::aligned(4096)]]
static paging::page_table_entry user_pte[paging::USER_PT_SIZE][paging::NUM_PT_ENTRIES];

// The APIC uses MMIO at a very specific memory address, so this needs to be mapped
[[gnu::aligned(4096)]]
static paging::page_table_entry apic_pte[paging::NUM_PT_ENTRIES];

void init_kernel_pde(paging::page_directory_entry* pde, paging::page_table_entry* pte) {
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
    pde->addr = paging::virt_to_phys(pte) >> 12;
}

void init_pdt() {
    // Kernel low address space (0-32MB)
    for (std::uint32_t i = 0; i < paging::KERNEL_LOW_PT_SIZE; i++) {
        auto* pde = &pdt[paging::KERNEL_LOW_PDE_START + i];
        auto* pte = kernel_low_pte[i];

        init_kernel_pde(pde, pte);
    }

    // Kernel high address space (3GB+)
    for (std::uint32_t i = 0; i < paging::KERNEL_HIGH_PT_SIZE; i++) {
        auto* pde = &pdt[paging::KERNEL_HIGH_PDE_START + i];
        auto* pte = kernel_high_pte[i];

        init_kernel_pde(pde, pte);
    }

    // Userspace (32-64MB)
    for (std::uint32_t i = 0; i < paging::USER_PT_SIZE; i++) {
        auto* pde = &pdt[paging::USER_PDE_START + i];
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
        pde->addr = paging::virt_to_phys(pte) >> 12;
    }

    // The APIC uses memory mapped IO calls at address (0xFEE00000)
    // Until we have all of pdt mapped, this needs to be explicitly set
    // 0xFEE00000 >> 22 = 0x3FB (1019) = PDE index
    // 0xFEE00000 >> 12 = 0xFEE00 & 0x3FB = 0x000 = PTE index
    auto* apic_pde = &pdt[paging::APIC_PDE_START];

    apic_pde->p = 1;
    apic_pde->rw = 1;
    apic_pde->us = 0;
    apic_pde->pwt = 1;
    apic_pde->pcd = 1;
    apic_pde->a = 0;
    apic_pde->ign = 0;
    apic_pde->ps = 0;
    apic_pde->avl = 0;
    apic_pde->addr = paging::virt_to_phys(apic_pte) >> 12;
}

void init_pte() {
    // Kernel low address space (0-32MB)
    for (std::uint32_t table = 0; table < paging::KERNEL_LOW_PT_SIZE; table++) {
        for (std::uint32_t i = 0; i < paging::NUM_PT_ENTRIES; i++) {
            std::uint32_t page_frame = (table * paging::NUM_PT_ENTRIES) + i;
            
            kernel_low_pte[table][i].p = 1;
            kernel_low_pte[table][i].rw = 1;
            kernel_low_pte[table][i].us = 0;
            kernel_low_pte[table][i].addr = page_frame;
        }
    }

    // Kernel high address space (3GB+)
    for (std::uint32_t table = 0; table < paging::KERNEL_HIGH_PT_SIZE; table++) {
        for (std::uint32_t i = 0; i < paging::NUM_PT_ENTRIES; i++) {
            std::uint32_t page_frame = (table * paging::NUM_PT_ENTRIES) + i;
            
            kernel_high_pte[table][i].p = 1;
            kernel_high_pte[table][i].rw = 1;
            kernel_high_pte[table][i].us = 0;
            kernel_high_pte[table][i].addr = page_frame;
        }
    }

    // Userspace (32-64MB)
    for (std::uint32_t table = 0; table < paging::USER_PT_SIZE; table++) {
        for (std::uint32_t i = 0; i < paging::NUM_PT_ENTRIES; i++) {
            std::uint32_t base_frame = paging::KERNEL_LOW_PT_SIZE * paging::NUM_PT_ENTRIES;
            std::uint32_t page_frame = base_frame + (table * paging::NUM_PT_ENTRIES) + i;
            
            user_pte[table][i].p = 1;
            user_pte[table][i].rw = 1;
            user_pte[table][i].us = 1; // User accessible
            user_pte[table][i].addr = page_frame;
        }
    }

    // APIC address space
    for (std::uint32_t i = 0; i < paging::NUM_PT_ENTRIES; i++) {
        apic_pte[i].p = 1;
        apic_pte[i].rw = 1;
        apic_pte[i].us = 0;
        apic_pte[i].pcd = 1;            // Disable cache for MMIO
        apic_pte[i].addr = 0xFEE00 + i; // physical address 0xFEE00000 >> 12
    }
}

void enable_paging() {
    std::uint32_t cr0;
    std::uintptr_t pdt_addr = paging::virt_to_phys(pdt);
    
    asm volatile("mov %0, %%cr3" : : "r"(pdt_addr) : "memory");
    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    cr0 |= 0x80000000; // set bit 31 to enable paging

    asm volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

void paging::init() {
    kernel::log::init_start("Paging");
    
    init_pdt();
    init_pte();

    /*
    printf("pdt addr: ");
    terminal_putint((int)(void*)pdt);
    printf("\n");

    printf("page_tables[4]: ");
    terminal_putint((int)(void*)page_tables[4]);
    printf("\n");

    printf("pdt[4].p: ");
    terminal_putint(pdt[4].p);
    printf("\n");

    printf("pdt[4].addr: ");
    terminal_putint(pdt[4].addr);
    printf("\n");

    printf("page_tables[4][510].p: ");
    terminal_putint(page_tables[4][510].p);
    printf("\n");

    printf("page_tables[4][510].addr: ");
    terminal_putint(page_tables[4][510].addr);
    printf("\n");

    printf("pt_apic addr: ");
    terminal_putuint((uint32_t)(void*)(pt_apic));
    printf("\n");

    printf("pdt[1019].p =: ");
    terminal_putuint(pdt[1019].p);
    printf("\n");

    printf("pdt[1019].addr =: ");
    terminal_putuint(pdt[1019].addr);
    printf("\n");

    printf("p = ");
    terminal_putuint(pt_apic[0].p);
    printf("\n");

    printf("rw = ");
    terminal_putuint(pt_apic[0].rw);
    printf("\n");

    printf("addr = ");
    terminal_putuint(pt_apic[0].addr);
    printf("\n");

    printf("pcd = ");
    terminal_putuint(pt_apic[0].pcd);
    printf("\n");
    */
    
    enable_paging();

    kernel::log::init_end("Paging");
}
