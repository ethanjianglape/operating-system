#include <cstdint>
#include <kernel/arch/arch.hpp>

#include <cstddef>
#include <concepts>

namespace pmm {
    // For now, the PMM will have a hard coded upper limit of 2GiB
    // attempting to access beyond 2GiB will just truncate
    constexpr std::size_t MAX_MEMORY_BYTES = 2'147'483'648;
    constexpr std::size_t FRAME_SIZE = arch::vmm::PAGE_SIZE;
    constexpr std::size_t MAX_NUM_FRAMES = MAX_MEMORY_BYTES / FRAME_SIZE;

    constexpr std::size_t FRAME_BITMAP_ENTRY_SIZE = sizeof(std::size_t) * 8;
    constexpr std::size_t FRAME_BITMAP_SIZE = MAX_NUM_FRAMES / FRAME_BITMAP_ENTRY_SIZE;
    constexpr std::size_t FRAME_FREE = 0;
    constexpr std::size_t FRAME_USED = 1;

    void init();

    void add_free_memory(std::size_t addr, std::size_t len);
    void set_addr_free(std::size_t addr, std::size_t length);
    
    std::size_t get_total_memory();

    void free_frame(std::uintptr_t phys);
    void free_contiguous_frames(std::uintptr_t phys, std::size_t count);

    void* alloc_frame();
    void* alloc_contiguous_frames(std::size_t num_frames);

    template <std::unsigned_integral T>
    T alloc_contiguous_frames(std::size_t num_frames) {
        return reinterpret_cast<T>(alloc_contiguous_frames(num_frames));
    }

    template <typename T>
    T alloc_frame() { return reinterpret_cast<T>(alloc_frame()); }
}
