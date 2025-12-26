#include "vmm.hpp"

#include <fmt/fmt.hpp>
#include <log/log.hpp>
#include <memory/memory.hpp>
#include <memory/pmm.hpp>

#include <cstddef>
#include <cstdint>

namespace x86_64::vmm {
    static PageTableEntry* pml4;

    static std::uint8_t* kernel_heap;
    
    static std::uintptr_t hhdm_offset;

    std::uintptr_t get_hhdm_offset() { return hhdm_offset; }

    void make_pte(PageTableEntry* pte, std::uintptr_t phys, std::uint64_t flags) {
        pte->p = (flags & PAGE_PRESENT) ? 1 : 0;
        pte->rw = (flags & PAGE_WRITE) ? 1 : 0;
        pte->us = (flags & PAGE_USER) ? 1 : 0;
        pte->pcd = (flags & PAGE_CACHE_DISABLE) ? 1 : 0;
        pte->addr = phys >> 12;
    }

    void zero_page(std::uintptr_t* virt) {
        for (std::size_t i = 0; i < NUM_PT_ENTRIES; i++) {
            virt[i] = 0x0;
        }
    }

    std::uintptr_t map_hddm_page(std::uintptr_t phys, std::uint32_t flags) {
        std::uintptr_t virt = get_hhdm_offset() + phys;

        map_page(virt, phys, flags);

        return virt;
    }

    void map_page(std::uintptr_t virt, std::uintptr_t phys, std::uint32_t flags) {
        const std::uintptr_t pml4_idx = (virt >> 39) & 0x1FF;
        const std::uintptr_t pdpt_idx = (virt >> 30) & 0x1FF;
        const std::uintptr_t pd_idx   = (virt >> 21) & 0x1FF;
        const std::uintptr_t pt_idx   = (virt >> 12) & 0x1FF;

        if (!pml4[pml4_idx].p) {
            // Allocate a new 4KiB physical frame for this entry
            auto page_phys = pmm::alloc_frame<std::uintptr_t>();

            // Create the missing pte entry at this index
            make_pte(&pml4[pml4_idx], page_phys, flags | PAGE_PRESENT | PAGE_WRITE);

            // Zero out the page in the next layer of the page table
            zero_page(phys_to_virt<std::uintptr_t*>(page_phys));
        }

        auto* pdpt = phys_to_virt<PageTableEntry*>(pml4[pml4_idx].addr << 12);

        if (!pdpt[pdpt_idx].p) {
            // Allocate a new 4KiB physical frame for this entry
            auto page_phys = pmm::alloc_frame<std::uintptr_t>();

            // Create the missing pte entry at this index
            make_pte(&pdpt[pdpt_idx], page_phys, flags | PAGE_PRESENT | PAGE_WRITE);

            // Zero out the page in the next layer of the page table
            zero_page(phys_to_virt<std::uintptr_t*>(page_phys));
        }

        auto* pd = phys_to_virt<PageTableEntry*>(pdpt[pdpt_idx].addr << 12);

        if (!pd[pd_idx].p) {
            // Allocate a new 4KiB physical frame for this entry
            auto page_phys = pmm::alloc_frame<std::uintptr_t>();

            // Create the missing pte entry at this index
            make_pte(&pd[pd_idx], page_phys, flags | PAGE_PRESENT | PAGE_WRITE);

            // Zero out the page in the next layer of the page table
            zero_page(phys_to_virt<std::uintptr_t*>(page_phys));
        }

        auto* pt = phys_to_virt<PageTableEntry*>(pd[pd_idx].addr << 12);

        make_pte(&pt[pt_idx], phys, flags | PAGE_PRESENT | PAGE_WRITE);

        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    }

    void* alloc_contiguous_memory(std::size_t bytes) {
        std::size_t total = bytes + sizeof(std::size_t);
        std::size_t pages = (total / PAGE_SIZE) + 1;

        void* block = alloc_contiguous_pages(pages);

        *static_cast<std::size_t*>(block) = pages;

        return static_cast<std::uint8_t*>(block) + sizeof(std::size_t);
    }

    void* alloc_contiguous_pages(std::size_t pages) {
        auto phys = pmm::alloc_contiguous_frames<std::uintptr_t>(pages);

        return phys_to_virt<void*>(phys);
    }

    void free_contiguous_memory(void* virt) {
        if (virt == nullptr) {
            return;
        }

        void* block = static_cast<std::uint8_t*>(virt) - sizeof(std::size_t);
        auto pages = *static_cast<std::size_t*>(block);

        pmm::free_contiguous_frames(virt_to_phys(block), pages);
    }

    // Set our local pml4 to point to the pml4 created by Limine which
    // is stored in cr3 as a physical address
    void init_pml4() {
        std::uint64_t cr3;
        asm volatile("mov %%cr3, %0" : "=r"(cr3));

        // The bottom 12 bits of the pml4 physical address should be ignored
        constexpr std::uint64_t bottom_12_mask = ~0xFFF;
        pml4 = phys_to_virt<PageTableEntry*>(cr3 & bottom_12_mask);

        log::info("VMM pml4 addr = ", fmt::hex{pml4});
    }

    void init_userspace() {
        auto code_phys = pmm::alloc_contiguous_frames<std::uintptr_t>(8);
        auto data_phys = pmm::alloc_contiguous_frames<std::uintptr_t>(8);

        map_page(0x00400000, code_phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        map_page(0x00800000, data_phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

        log::info("Mapped user code at virt ", fmt::hex{0x00400000}, " to phys ", fmt::hex{code_phys});
        log::info("Mapped user data at virt ", fmt::hex{0x00800000}, " to phys ", fmt::hex{data_phys});
    }

    void init(std::uintptr_t offset) {
        log::init_start("VMM");

        hhdm_offset = offset;
        kernel_heap = reinterpret_cast<std::uint8_t*>(hhdm_offset);

        log::info("VMM HHDM addr = ", fmt::hex{hhdm_offset});
        log::info("Kernel heap   = ", fmt::hex{kernel_heap});

        init_pml4();
        init_userspace();

        log::init_end("VMM");
    }
}


