#include "paging.hpp"

#include <stdio.h>

#include <kernel/tty.h>

using namespace i386;

[[gnu::aligned(4096)]]
static paging::page_directory_entry pdt[paging::NUM_PDT_ENTRIES];

[[gnu::aligned(4096)]]
static paging::page_table_entry page_tables[8][paging::NUM_PT_ENTRIES]; // 8 pages, Maps 0-32 MB

[[gnu::aligned(4096)]]
static paging::page_table_entry pt_apic[paging::NUM_PT_ENTRIES];

void init_pdt() {
    for (std::uint32_t i = 0; i < 8; i++) {
        paging::page_directory_entry* entry = &pdt[i];

        entry->p = 1;
        entry->rw = 1;
        entry->us = 0;
        entry->pwt = 0;
        entry->pcd = 0;
        entry->a = 0;
        entry->ign = 0;
        entry->ps = 0;
        entry->avl = 0;
        entry->addr = reinterpret_cast<std::uintptr_t>(page_tables[i]) >> 12;
    }

    // The APIC uses memory mapped IO calls at address (0xFEE00000)
    // Until we have all of pdt mapped, this needs to be explicitly set
    // 0xFEE00000 >> 22 = 0x3FB (1019) = PDE index
    // 0xFEE00000 >> 12 = 0xFEE00 & 0x3FB = 0x000 = PTE index
    paging::page_directory_entry* pdt_apic = &pdt[1019];

    pdt_apic->p = 1;
    pdt_apic->rw = 1;
    pdt_apic->us = 0;
    pdt_apic->pwt = 1;
    pdt_apic->pcd = 1;
    pdt_apic->a = 0;
    pdt_apic->ign = 0;
    pdt_apic->ps = 0;
    pdt_apic->avl = 0;
    pdt_apic->addr = reinterpret_cast<std::uintptr_t>(pt_apic) >> 12;
}

void init_pte() {
    for (std::uint32_t table = 0; table < 8; table++) {
        for (std::uint32_t i = 0; i < paging::NUM_PT_ENTRIES; i++) {
            std::uint32_t page_frame = (table * paging::NUM_PT_ENTRIES) + i;
            
            page_tables[table][i].p = 1;
            page_tables[table][i].rw = 1;
            page_tables[table][i].us = 0;
            page_tables[table][i].addr = page_frame;
        }
    }

    for (std::uint32_t i = 0; i < paging::NUM_PT_ENTRIES; i++) {
        pt_apic[i].p = 1;
        pt_apic[i].rw = 1;
        pt_apic[i].us = 0;
        pt_apic[i].pcd = 1;        // Disable cache for MMIO
        pt_apic[i].addr = 0xFEE00 + i; // physical address 0xFEE00000 >> 12
    }
}

void enable_paging() {
    std::uint32_t cr0;
    
    asm volatile("mov %0, %%cr3" : : "r"(pdt) : "memory");
    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    cr0 |= 0x80000000; // set bit 31 to enable paging

    asm volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

void paging::init() {
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
}
