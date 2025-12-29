#pragma once

#include <cstddef>
#include <cstdint>

namespace slab {
    struct Slab {
        std::uint64_t magic;           // Magic value to identify slab headers
        void* free_head;               // Pointer to next free slab chunk
        Slab* next_slab;               // Pointer to next slab in the slab linked list
        Slab* prev_slab;               // Pointer to prev slab in the slab linked list
        std::uint8_t size_class_index; // Index of the SizeClass this slab belongs to
        std::uint8_t free_chunks;      // Number of remaining free chunks
    };

    struct SizeClass {
        std::uint8_t index;           // Unique index of this SizeClass
        std::size_t size;             // Size of this slab class (32, 64, 128, etc)
        std::size_t num_slabs;        // Total number of slabs in the linked list
        Slab* first_slab;             // Pointer to first slab in the slab linked list
        std::uint8_t chunks_per_slab; // Number of chunks per slab (constant for this size class)
    };

    bool can_alloc(std::size_t bytes);

    bool is_slab(void* addr);

    void* alloc(std::size_t size);

    void free(void* addr);
}
