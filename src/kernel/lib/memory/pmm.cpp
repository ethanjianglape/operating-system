/**
 * @file pmm.cpp
 * @brief Physical Memory Manager — bitmap-based frame allocator.
 *
 * The PMM tracks which 4KiB physical memory frames are free or in use.
 * It uses a bitmap where each bit represents one frame:
 *   - 0 = free
 *   - 1 = used
 *
 * During boot, the bootloader (Limine) tells us which memory regions are
 * usable. We mark those as free in the bitmap, then allocate from them
 * as needed for page tables, kernel data structures, etc.
 *
 * This is a simple first-fit allocator. For contiguous allocations, it
 * scans for consecutive free frames.
 */

#include <memory/pmm.hpp>
#include <kpanic/kpanic.hpp>
#include <fmt/fmt.hpp>
#include <log/log.hpp>
#include <arch.hpp>

#include <cstdint>
#include <cstddef>

namespace pmm {
    // Bitmap where each bit represents a 4KiB frame (0 = free, 1 = used)
    static std::size_t frame_bitmap[FRAME_BITMAP_SIZE];
    static std::size_t frame_bitmap_start;
    static std::size_t frame_bitmap_end;

    static std::size_t total_memory;
    static std::size_t total_frames;
    static std::size_t free_frames;
    
    bool is_frame_free(std::size_t frame) {
        const std::size_t index = frame / FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t offset = frame % FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t entry = frame_bitmap[index];
        const std::size_t value = entry & (FRAME_USED << offset);

        return value == FRAME_FREE;
    }

    void set_frame_used(std::size_t frame) {
        const std::size_t index = frame / FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t offset = frame % FRAME_BITMAP_ENTRY_SIZE;

        frame_bitmap[index] |= (FRAME_USED << offset);
        free_frames--;
    }

    void set_frame_free(std::size_t frame) {
        const std::size_t index = frame / FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t offset = frame % FRAME_BITMAP_ENTRY_SIZE;
        const std::size_t mask = ~(FRAME_USED << offset);

        frame_bitmap[index] &= mask;
        free_frames++;
    }

    void init() {
        // all pages set to used by default
        for (std::size_t i = 0; i < FRAME_BITMAP_SIZE; i++) {
            frame_bitmap[i] = 0xFFFFFFFFFFFFFFFF;
        }

        total_memory = 0;
        total_frames = 0;
        free_frames = 0;
    }

    /**
     * @brief Registers a region of physical memory as available for allocation.
     *
     * Called during boot for each usable memory region reported by Limine.
     * Regions beyond MAX_MEMORY_BYTES are truncated or ignored.
     *
     * @param addr Physical start address of the region.
     * @param len Length of the region in bytes.
     */
    void add_free_memory(std::size_t addr, std::size_t len) {
        std::uint64_t end = addr + len;

        if (end >= MAX_MEMORY_BYTES) {
            if (addr >= MAX_MEMORY_BYTES) {
                log::warn("Ignoring memory region at ", fmt::hex{addr}, " (beyond max)");
                return;
            }

            log::warn("Truncating memory region from ", fmt::hex{end}, " to ", fmt::hex{MAX_MEMORY_BYTES});
            len = MAX_MEMORY_BYTES - addr;
        }

        total_memory += len;
        total_frames += (len / FRAME_SIZE) + 1;
        free_frames += total_frames;

        set_addr_free(addr, len);
        set_frame_used(0);
    }

    std::size_t get_total_memory() {
        return total_memory;
    }

    std::size_t get_free_frames() {
        return free_frames;
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

    void free_frame(std::uintptr_t phys) {
        std::size_t frame = phys / FRAME_SIZE;

        set_frame_free(frame);
    }

    void free_contiguous_frames(std::uintptr_t phys, std::size_t count) {
        std::size_t frame = phys / FRAME_SIZE;

        for (std::size_t i = 0; i < count; i++) {
            set_frame_free(frame + i);
        }
    }

    /**
     * @brief Allocates a single 4KiB physical frame.
     *
     * Uses first-fit search starting from the last allocation point.
     *
     * @return Physical address of the allocated frame.
     * @throws Panics if no free frames are available.
     */
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

        kpanic("PMM: Out of physical memory");

        return nullptr;
    }

    /**
     * @brief Allocates multiple contiguous physical frames.
     *
     * Scans the entire bitmap for a run of consecutive free frames.
     * Slower than single-frame allocation but necessary for DMA buffers
     * and other hardware that requires physically contiguous memory.
     *
     * @param num_frames Number of contiguous frames to allocate.
     * @return Physical address of the first frame.
     * @throws Panics if not enough contiguous frames are available.
     */
    void* alloc_contiguous_frames(std::size_t num_frames) {
        std::size_t consecutive = 0;
        std::size_t start_frame = 0;

        for (std::size_t frame = 0; frame < total_frames; frame++) {
            if (is_frame_free(frame)) {
                if (consecutive == 0) {
                    start_frame = frame;
                }

                consecutive++;

                if (consecutive >= num_frames) {
                    for (std::size_t i = start_frame; i < start_frame + num_frames; i++) {
                        set_frame_used(i);
                    }

                    return reinterpret_cast<void*>(start_frame * FRAME_SIZE);
                }
            } else {
                consecutive = 0;
            }
        }

        kpanic("PMM: Out of physical memory");
    }
}
