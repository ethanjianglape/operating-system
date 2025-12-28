#pragma once

#include <cstddef>
#include <cstdint>

namespace slab {
    constexpr std::size_t MAX_SLABS_PER_CLASS = 64;
    
    struct Slab {
        std::uint64_t magic;
        void* page;
        void* free_head;
        std::size_t size;
        std::size_t free_count;
    };

    struct SizeClass {
        std::size_t size;
        std::size_t count;
        Slab* slabs[MAX_SLABS_PER_CLASS];
    };

    bool can_alloc(std::size_t size);

    bool is_slab(void* addr);

    void* alloc(std::size_t size);

    void free(void* addr);
}
