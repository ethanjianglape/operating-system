#pragma once

#include <cstddef>
#include <cstdint>

namespace slab {
    struct Slab {
        std::uint64_t magic; // Magic value to identify slab headers
        void* free_head;     // Pointer to next free slab chunk
        std::size_t size;    // Size of this slab (32, 64, 128, etc)
        Slab* next_slab;     // Pointer to next slab in the slab linked list
    };

    struct SizeClass {
        std::size_t size; // Size of this slab class (32, 64, 128, etc)
        Slab* first_slab; // Pointer to first slab in the slab linked list
    };

    bool can_alloc(std::size_t size);

    bool is_slab(void* addr);

    void* alloc(std::size_t size);

    void free(void* addr);
}
