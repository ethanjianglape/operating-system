/**
 * @file vmm.cpp
 * @brief Virtual Memory Manager — x86-64 paging and address space management.
 *
 * This module manages virtual memory through x86-64's 4-level page tables.
 * It provides functions to map/unmap pages, allocate kernel memory, and
 * create/switch address spaces for user processes.
 *
 * Page Table Hierarchy (4-level paging):
 *
 *   Virtual Address (48 bits used):
 *   ┌────────┬────────┬────────┬────────┬──────────────┐
 *   │ PML4   │ PDPT   │  PD    │  PT    │    Offset    │
 *   │ [47:39]│ [38:30]│ [29:21]│ [20:12]│    [11:0]    │
 *   └────────┴────────┴────────┴────────┴──────────────┘
 *      9 bits   9 bits   9 bits   9 bits    12 bits
 *
 *   CR3 → PML4 → PDPT → PD → PT → Physical Page
 *
 * Higher Half Direct Map (HHDM):
 *
 *   Limine maps all physical memory at a fixed virtual offset (typically
 *   0xFFFF800000000000). This lets us access any physical address by simply
 *   adding the HHDM offset. We use this to access page tables themselves,
 *   since they're stored in physical memory.
 *
 *   phys_to_virt(phys) = phys + hhdm_offset
 *   virt_to_phys(virt) = virt - hhdm_offset  (for HHDM addresses)
 */

#include "vmm.hpp"

#include <fmt/fmt.hpp>
#include <log/log.hpp>
#include <memory/memory.hpp>
#include <memory/pmm.hpp>

#include <cstddef>
#include <cstdint>

namespace x86_64::vmm {
    static PageTableEntry* kernel_pml4;

    static std::uintptr_t hhdm_offset;

    std::uintptr_t get_hhdm_offset() { return hhdm_offset; }

    template <typename T>
    std::uintptr_t hhdm_virt_to_phys(T addr) {
        return reinterpret_cast<std::uintptr_t>(addr) - get_hhdm_offset();
    }

    template <typename T>
    T phys_to_virt(std::unsigned_integral auto phys) {
        return reinterpret_cast<T>(phys + get_hhdm_offset());
    }

    /**
     * @brief Walks the page table hierarchy to find the PTE for a virtual address.
     * @param virt The virtual address to look up.
     * @return Pointer to the page table entry, or nullptr if not mapped.
     */
    PageTableEntry* get_pte(std::uintptr_t virt){
        const std::uintptr_t pml4_idx = (virt >> 39) & 0x1FF;
        const std::uintptr_t pdpt_idx = (virt >> 30) & 0x1FF;
        const std::uintptr_t pd_idx   = (virt >> 21) & 0x1FF;
        const std::uintptr_t pt_idx   = (virt >> 12) & 0x1FF;

        if (!kernel_pml4[pml4_idx].p) {
            return nullptr;
        }
        
        auto* pdpt = phys_to_virt<PageTableEntry*>(kernel_pml4[pml4_idx].addr << 12);

        if (!pdpt[pdpt_idx].p) {
            return nullptr;
        }
        
        auto* pd = phys_to_virt<PageTableEntry*>(pdpt[pdpt_idx].addr << 12);

        if (!pd[pd_idx].p) {
            return nullptr;
        }
        
        auto* pt = phys_to_virt<PageTableEntry*>(pd[pd_idx].addr << 12);

        if (!pt[pt_idx].p) {
            return nullptr;
        }

        return &pt[pt_idx];
    }

    template <typename T>
    std::uintptr_t virt_to_phys(T virt) {
        PageTableEntry* pte = get_pte(virt);

        if (pte == nullptr) {
            log::warn("virt_to_phys called on unmapped address: ", fmt::hex{virt});
            return 0;
        }

        return pte->addr << 12;
    }

    /**
     * @brief Populates a page table entry with a physical address and flags.
     * @param pte Pointer to the page table entry to modify.
     * @param phys Physical address to map (must be page-aligned).
     * @param flags Page flags (PAGE_PRESENT, PAGE_WRITE, PAGE_USER, etc.).
     */
    void make_pte(PageTableEntry* pte, std::uintptr_t phys, std::uint64_t flags) {
        pte->p = (flags & PAGE_PRESENT) ? 1 : 0;
        pte->rw = (flags & PAGE_WRITE) ? 1 : 0;
        pte->us = (flags & PAGE_USER) ? 1 : 0;
        pte->pcd = (flags & PAGE_CACHE_DISABLE) ? 1 : 0;
        pte->addr = phys >> 12; // the bottom 12 bits of the physical address are not stored
    }

    void zero_page(std::uintptr_t* virt) {
        for (std::size_t i = 0; i < NUM_PT_ENTRIES; i++) {
            virt[i] = 0x0;
        }
    }

    std::uintptr_t map_hddm_page(std::uintptr_t phys, std::uint32_t flags) {
        std::uintptr_t virt = get_hhdm_offset() + phys;

        map_kpage(virt, phys, flags);

        return virt;
    }

    /**
     * @brief Ensures a page table entry points to a valid next-level table.
     *
     * If the entry is not present, allocates a new physical frame, zeros it,
     * and populates the entry. Used during page table walks to lazily create
     * intermediate tables (PDPT, PD, PT) as needed.
     *
     * @param pte Pointer to the page table entry to check/populate.
     * @param flags Flags to set on the entry if newly allocated.
     */
    void ensure_table_present(PageTableEntry* pte, std::uint32_t flags) {
        if (!pte->p) {
            // Allocate a new 4KiB physical frame for this entry
            auto page_phys = pmm::alloc_frame<std::uintptr_t>();

            // Create the missing pte entry at this index
            make_pte(pte, page_phys, flags);

            // Zero out the page in the next layer of the page table
            zero_page(phys_to_virt<std::uintptr_t*>(page_phys));
        }
    }

    /**
     * @brief Maps a virtual address to a physical address in the given page table.
     *
     * Walks the 4-level page table, creating intermediate tables as needed,
     * then creates the final mapping. Invalidates the TLB entry for this address.
     *
     * @param pml4 The top-level page table (PML4) to modify.
     * @param virt Virtual address to map (must be page-aligned).
     * @param phys Physical address to map to (must be page-aligned).
     * @param flags Page flags (PAGE_PRESENT, PAGE_WRITE, PAGE_USER, etc.).
     */
    void map_page(PageTableEntry* pml4, std::uintptr_t virt, std::uintptr_t phys, std::uint32_t flags) {
        const std::uintptr_t pml4_idx = (virt >> 39) & 0x1FF;
        const std::uintptr_t pdpt_idx = (virt >> 30) & 0x1FF;
        const std::uintptr_t pd_idx   = (virt >> 21) & 0x1FF;
        const std::uintptr_t pt_idx   = (virt >> 12) & 0x1FF;

        ensure_table_present(&pml4[pml4_idx], flags | PAGE_PRESENT | PAGE_WRITE);
        auto* pdpt = phys_to_virt<PageTableEntry*>(pml4[pml4_idx].addr << 12);

        ensure_table_present(&pdpt[pdpt_idx], flags | PAGE_PRESENT | PAGE_WRITE);
        auto* pd = phys_to_virt<PageTableEntry*>(pdpt[pdpt_idx].addr << 12);

        ensure_table_present(&pd[pd_idx], flags | PAGE_PRESENT | PAGE_WRITE);
        auto* pt = phys_to_virt<PageTableEntry*>(pd[pd_idx].addr << 12);

        make_pte(&pt[pt_idx], phys, flags | PAGE_PRESENT | PAGE_WRITE);

        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    }

    void map_kpage(std::uintptr_t virt, std::uintptr_t phys, std::uint32_t flags) {
        map_page(kernel_pml4, virt, phys, flags);
    }

    /**
     * @brief Unmaps a virtual address and frees its physical frame.
     * @param virt Virtual address to unmap.
     */
    void unmap_page(std::uintptr_t virt) {
        PageTableEntry* pte = get_pte(virt);

        if (pte == nullptr) {
            log::warn("Attempt to unmap virt addr that is not mapped: ", fmt::hex{virt});
            return;
        }
        
        auto phys = pte->addr << 12;

        *pte = {};

        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");

        pmm::free_frame(phys);
    }

    void* alloc_kpage() {
        auto phys = pmm::alloc_frame<std::uintptr_t>();
        return phys_to_virt<void*>(phys);
    }

    void free_kpage(void* virt) {
        if (virt == nullptr) {
            return;
        }
        pmm::free_frame(hhdm_virt_to_phys(virt));
    }

    void* alloc_contiguous_kmem(std::size_t bytes) {
        std::size_t total = bytes + sizeof(std::size_t);
        std::size_t num_pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;

        auto phys = pmm::alloc_contiguous_frames<std::uintptr_t>(num_pages);
        void* block = phys_to_virt<void*>(phys);

        *static_cast<std::size_t*>(block) = num_pages;

        return static_cast<std::uint8_t*>(block) + sizeof(std::size_t);
    }

    void free_contiguous_kmem(void* virt) {
        if (virt == nullptr) {
            return;
        }

        void* block = static_cast<std::uint8_t*>(virt) - sizeof(std::size_t);
        auto num_pages = *static_cast<std::size_t*>(block);

        pmm::free_contiguous_frames(hhdm_virt_to_phys(block), num_pages);
    }

    std::size_t map_mem_at(PageTableEntry* pml4, std::uintptr_t virt, std::size_t bytes, std::uint32_t flags) {
        std::size_t num_pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;

        for (std::size_t page = 0; page < num_pages; page++){
            auto phys = pmm::alloc_frame<std::uintptr_t>();

            map_page(pml4, virt + (page * PAGE_SIZE), phys, flags);
        }

        return num_pages;
    }

    void unmap_mem_at(std::uintptr_t virt, std::size_t num_pages) {
        for (std::size_t page = 0; page < num_pages; page++) {
            unmap_page(virt + (page * PAGE_SIZE));
        }
    }

    // Set our local pml4 to point to the pml4 created by Limine which
    // is stored in cr3 as a physical address
    void init_pml4() {
        std::uint64_t cr3;
        asm volatile("mov %%cr3, %0" : "=r"(cr3));

        // The bottom 12 bits of the pml4 physical address should be ignored
        constexpr std::uint64_t bottom_12_mask = ~0xFFF;
        kernel_pml4 = phys_to_virt<PageTableEntry*>(cr3 & bottom_12_mask);

        log::info("VMM pml4 addr = ", fmt::hex{kernel_pml4});
    }

    std::size_t get_kernel_pml4_index() {
        return (hhdm_offset >> 39) & 0x1FF;
    }

    /**
     * @brief Creates a new PML4 for a user process.
     *
     * Allocates a new page table and copies the kernel's higher-half mappings
     * (from HHDM index onwards) so the kernel is accessible from user space.
     * The lower half is left empty for user mappings.
     *
     * @return Pointer to the new PML4 (virtual address).
     */
    PageTableEntry* create_user_pml4() {
        auto phys = pmm::alloc_frame<std::uintptr_t>();
        auto* virt = phys_to_virt<std::uintptr_t*>(phys);
        auto* new_pml4 = reinterpret_cast<PageTableEntry*>(virt);

        zero_page(virt);

        std::size_t kernel_start = get_kernel_pml4_index();

        for (std::size_t i = kernel_start; i < NUM_PT_ENTRIES; i++) {
            new_pml4[i] = kernel_pml4[i];
        }

        return new_pml4;
    }

    void switch_pml4(PageTableEntry* pml4) {
        asm volatile("mov %0, %%cr3" : : "r"(hhdm_virt_to_phys(pml4)) : "memory");
    }

    void switch_kernel_pml4() {
        switch_pml4(kernel_pml4);
    }

    /**
     * @brief Initializes the Virtual Memory Manager.
     * @param offset The HHDM offset provided by Limine.
     */
    void init(std::uintptr_t offset) {
        log::init_start("VMM");

        hhdm_offset = offset;

        log::info("VMM HHDM addr = ", fmt::hex{hhdm_offset});

        init_pml4();

        log::init_end("VMM");
    }
}


