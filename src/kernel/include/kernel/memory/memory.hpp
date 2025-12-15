#pragma once

#include <kernel/arch/arch.hpp>

#include <cstddef>

namespace kernel {
    void* kmalloc(std::size_t size);
    
    void kfree(void* ptr);

    namespace pmm {
        // For now, the PMM will have a hard coded upper limit of 2GiB
        // attempting to access beyond 2GiB will not be allowed
        inline constexpr std::size_t MAX_MEMORY_BYTES = 2'147'483'648;

        inline constexpr std::size_t FRAME_SIZE = arch::vmm::PAGE_SIZE;

        // 2GiB as the max amount of system memory gives us 52437 physical frames
        inline constexpr std::size_t MAX_NUM_FRAMES = MAX_MEMORY_BYTES / FRAME_SIZE;

        inline constexpr std::size_t KERNEL_CODE_START_ADDR = 0x00200000; // Kernel starts at phys addr 2MiB
        inline constexpr std::size_t KERNEL_CODE_SIZE       = 0x00200000; // Kernel is currently 2MiB long
        inline constexpr std::size_t KERNEL_CODE_END_ADDR   = KERNEL_CODE_START_ADDR + KERNEL_CODE_SIZE;

        inline constexpr std::size_t KERNEL_DATA_START_ADDR = KERNEL_CODE_END_ADDR + 1;
        inline constexpr std::size_t KERNEL_DATA_SIZE       = 0x02000000; // 32MiB
        inline constexpr std::size_t KERNEL_DATA_END_ADDR   = KERNEL_DATA_START_ADDR + KERNEL_DATA_SIZE;

        inline constexpr std::size_t KERNEL_CODE_START_FRAME = KERNEL_CODE_START_ADDR / FRAME_SIZE;
        inline constexpr std::size_t KERNEL_CODE_END_FRAME   = KERNEL_CODE_END_ADDR / FRAME_SIZE;

        inline constexpr std::size_t KERNEL_DATA_START_FRAME = KERNEL_DATA_START_ADDR / FRAME_SIZE;
        inline constexpr std::size_t KERNEL_DATA_END_FRAME   = KERNEL_DATA_END_ADDR / FRAME_SIZE;
        
        inline constexpr std::uint32_t FRAME_BITMAP_ENTRY_SIZE = sizeof(std::uint32_t) * 8;
        inline constexpr std::uint32_t FRAME_BITMAP_SIZE = MAX_NUM_FRAMES / FRAME_BITMAP_ENTRY_SIZE;
        inline constexpr std::uint32_t FRAME_FREE = 0;
        inline constexpr std::uint32_t FRAME_USED = 1;
        
        void init(std::size_t total_memory_bytes);

        void* alloc_frame();
    }
}
