#include <kernel/memory/memory.hpp>
#include <kernel/log/log.hpp>
#include <kernel/panic/panic.hpp>
#include <kernel/arch/arch.hpp>

#include <cstdint>
#include <cstddef>

extern "C" std::uint32_t kernel_physical_start;
extern "C" std::uint32_t kernel_physical_end;

namespace kernel::pmm {
    // 1 bit used per frame, 0 = free, 1 = used
    static std::uint32_t frame_bitmap[FRAME_BITMAP_SIZE];
    static std::uint32_t frame_bitmap_start;
    static std::uint32_t frame_bitmap_end;

    bool is_frame_free(std::size_t frame) {
        const std::size_t index = frame / FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t offset = frame % FRAME_BITMAP_ENTRY_SIZE;
        const std::uint32_t entry = frame_bitmap[index];
        const std::uint32_t value = entry & (FRAME_USED << offset);

        return value == FRAME_FREE;
    }

    void set_frame_used(std::size_t frame) {
        const std::size_t index = frame / FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t offset = frame % FRAME_BITMAP_ENTRY_SIZE;

        frame_bitmap[index] |= (FRAME_USED << offset);
    }

    void set_frame_free(std::size_t frame) {
        const std::size_t index = frame / FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t offset = frame % FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t mask = ~(FRAME_USED << offset);

        frame_bitmap[index] &= mask;
    }

    void init(std::size_t total_mem_bytes,
              std::size_t free_mem_addr,
              std::size_t free_mem_len) {
        log::init_start("Physical Memory Management");
        
        if (total_mem_bytes > MAX_MEMORY_BYTES) {
            log::warn("System booted with %u bytes of RAM, defaulting to 2GiB", total_mem_bytes);
            total_mem_bytes = MAX_MEMORY_BYTES;
        }

        if (free_mem_addr + free_mem_len >= total_mem_bytes) {
            kernel::panicf("Memory address %x (length=%x) is outside of available memory",
                           free_mem_addr,
                           free_mem_len);
        }

        log::info("Total system memory: %u bytes", total_mem_bytes);
        log::info("Free memory start: %x", free_mem_addr);
        log::info("Free memory length: %u bytes", free_mem_len);
        
        // all pages set to used by default
        for (std::size_t i = 0; i < FRAME_BITMAP_SIZE; i++) {
            frame_bitmap[i] = 0xFFFFFFFF;
        }

        // allocate the free memory range
        set_addr_free(free_mem_addr, free_mem_len);

        auto kernel_start_addr = reinterpret_cast<std::size_t>(&kernel_physical_start);
        auto kernel_end_addr = reinterpret_cast<std::size_t>(&kernel_physical_end);
        auto kernel_len = kernel_end_addr - kernel_start_addr;

        log::info("Kernel starts at %x", kernel_start_addr);
        log::info("Kernel length %u bytes", kernel_len);

        set_addr_used(kernel_start_addr, kernel_len);
        
        log::init_end("Physical Memory Management");
    }

    void set_addr_free(std::size_t addr, std::size_t length) {
        std::size_t frame_start = addr / FRAME_SIZE;
        std::size_t frame_end = (addr + length) / FRAME_SIZE;

        frame_bitmap_start = frame_start;
        frame_bitmap_end = frame_end;

        while (frame_start <= frame_end) {
            set_frame_free(frame_start++);
        }
    }

    void set_addr_used(std::size_t addr, std::size_t length) {
        std::size_t frame_start = addr / FRAME_SIZE;
        std::size_t frame_end = (addr + length) / FRAME_SIZE;

        while (frame_start <= frame_end) {
            set_frame_used(frame_start++);
        }
    }

    void* alloc_frame() {
        std::size_t frame = frame_bitmap_start;

        while (frame < frame_bitmap_end) {
            if (is_frame_free(frame)) {
                set_frame_used(frame);
                frame_bitmap_start = frame + 1;
                return (void*)(frame * FRAME_SIZE);
            }

            frame++;
        }

        kernel::panic("PMM: Out of physical memory");
    }
}
