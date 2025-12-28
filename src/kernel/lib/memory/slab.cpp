#include "log/log.hpp"
#include <memory/slab.hpp>
#include <arch.hpp>

#include <cstddef>
#include <cstdint>

namespace slab {
    static constexpr std::uint64_t SLAB_MAGIC = 0x51AB'51AB'51AB'51AB; // 51AB == slab in leetspeak

    static constexpr std::size_t SIZE_32 = 32;
    static constexpr std::size_t SIZE_64 = 64;
    static constexpr std::size_t SIZE_128 = 128;
    static constexpr std::size_t SIZE_256 = 256;
    static constexpr std::size_t SIZE_512 = 512;
    static constexpr std::size_t SIZE_1024 = 1024;
    
    static SizeClass classes[] = {
        {SIZE_32, 0, {}},
        {SIZE_64, 0, {}},
        {SIZE_128, 0, {}},
        {SIZE_256, 0, {}},
        {SIZE_512, 0, {}},
        {SIZE_1024, 0, {}}
    };

    bool can_alloc(std::size_t size) {
        return size <= 512;
    }

    SizeClass* get_size_class(std::size_t bytes) {
        if (bytes <= SIZE_32) { return &classes[0]; }
        if (bytes <= SIZE_64) { return &classes[1]; }
        if (bytes <= SIZE_128) { return &classes[2]; }
        if (bytes <= SIZE_256) { return &classes[3]; }
        if (bytes <= SIZE_512) { return &classes[4]; }
        if (bytes <= SIZE_1024) { return &classes[5]; }

        return nullptr;
    }

    Slab* create_slab(SizeClass* sc) {
        void* page = arch::vmm::alloc_contiguous_pages(1);
        Slab* slab = reinterpret_cast<Slab*>(page);

        auto* first_chunk = static_cast<std::uint8_t*>(page) + sizeof(Slab);
        std::size_t num_chunks = (arch::vmm::PAGE_SIZE - sizeof(Slab)) / sc->size;

        for (std::size_t i = 0; i < num_chunks - 1; i++) {
            std::uint8_t* this_chunk = first_chunk + (i * sc->size);
            std::uint8_t* next_chunk = this_chunk + sc->size;

            *(void**)this_chunk = next_chunk;
        }

        std::uint8_t* last_chunk = first_chunk + ((num_chunks - 1) * sc->size);
        *(void**)last_chunk = nullptr;
        
        slab->magic = SLAB_MAGIC;
        slab->size = sc->size;
        slab->free_head = first_chunk;
        slab->free_count = num_chunks;

        return slab;
    }

    bool is_slab(void* addr) {
        if (addr == nullptr) {
            return false;
        }

        auto* page = (void*)((std::uintptr_t)addr & ~0xFFF); // page align
        auto* slab = reinterpret_cast<Slab*>(page);

        return slab->magic == SLAB_MAGIC;
    }

    void* alloc(std::size_t size) {
        SizeClass* sc = get_size_class(size);
        Slab* slab = nullptr;

        for (std::size_t i = 0; i < sc->count; i++) {
            if (sc->slabs[i]->free_head != nullptr) {
                slab = sc->slabs[i];
                break;
            }
        }

        if (slab == nullptr) {
            slab = create_slab(sc);
            sc->count++;
            sc->slabs[sc->count] = slab;
        }

        void* chunk = slab->free_head;

        slab->free_head = *(void**)slab->free_head;

        return chunk;
    }

    void free(void* addr) {
        auto* ptr = static_cast<std::uint8_t*>(addr);
        auto* slab = reinterpret_cast<Slab*>(ptr - sizeof(Slab));

        *(void**)addr = slab->free_head;
        slab->free_head = addr;
    }
}
