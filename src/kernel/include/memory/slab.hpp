#pragma once

#include <cstddef>
#include <cstdint>

namespace slab {
    constexpr std::uint64_t SLAB_MAGIC = 0x51AB51AB51AB51AB; // 51AB == slab in leetspeak

    constexpr std::size_t SIZE_32 = 32;
    constexpr std::size_t SIZE_64 = 64;
    constexpr std::size_t SIZE_128 = 128;
    constexpr std::size_t SIZE_256 = 256;
    constexpr std::size_t SIZE_512 = 512;
    constexpr std::size_t SIZE_1024 = 1024;
    
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

    Slab* try_get_slab(void* addr);

    void* alloc(std::size_t size);

    void free(void* addr);

    // Diagnostic: returns total slab count across all size classes
    std::size_t total_slabs();
}
