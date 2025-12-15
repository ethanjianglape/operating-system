#include <kernel/memory/memory.hpp>
#include <kernel/log/log.hpp>
#include <kernel/panic/panic.hpp>
#include <kernel/arch/arch.hpp>

#include <cstdint>
#include <cstddef>

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

    void init(std::size_t total_memory_bytes) {
        log::init_start("Physical Memory Management");
        
        if (total_memory_bytes > MAX_MEMORY_BYTES) {
            log::warn("System booted with %u bytes of RAM, defaulting to 2GiB", total_memory_bytes);
            total_memory_bytes = MAX_MEMORY_BYTES;
        }
        
        // all pages set to used by default
        for (std::size_t i = 0; i < FRAME_BITMAP_SIZE; i++) {
            frame_bitmap[i] = 0xFFFFFFFF;
        }

        // mark the kernel code as used
        for (std::size_t frame = KERNEL_CODE_START_FRAME; frame <= KERNEL_CODE_END_FRAME; frame++) {
            set_frame_used(frame);
        }

        // mark kernel data as free
        for (std::size_t frame = KERNEL_DATA_START_FRAME; frame <= KERNEL_DATA_END_FRAME; frame++) {
            set_frame_free(frame);
        }

        frame_bitmap_start = KERNEL_DATA_START_FRAME;
        frame_bitmap_end = total_memory_bytes / FRAME_SIZE;
        
        log::init_end("Physical Memory Management");
    }

    void* alloc_page() {
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
