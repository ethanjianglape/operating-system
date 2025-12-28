#include <memory/slab.hpp>
#include <arch.hpp>

#include <cstddef>
#include <cstdint>

/**
 * Slab Allocator
 * ==============
 *
 * Organization:
 *   - Each SizeClass (32, 64, 128, 256, 512, 1024 bytes) maintains a
 *     linked list of Slabs
 *   - New slabs are inserted at the head for O(1) insertion
 *   - Allocation searches from the head, so the newest slab (most likely
 *     to have free chunks) is checked first
 *
 *   SizeClass (e.g. 64-byte)
 *       │
 *       ▼
 *   ┌────────┐    ┌───────┐    ┌────────┐
 *   │ Slab   │───▶│ Slab  │───▶│ Slab   │───▶ nullptr
 *   │(newest)│    │       │    │(oldest)│
 *   └────────┘    └───────┘    └────────┘
 *
 * Slab Page Layout:
 *
 *   A slab is a single 4KB page divided into fixed-size chunks.
 *   The Slab metadata is stored at the start of the page itself.
 *
 *   ┌────────────────────────────────────────────────────────────┐
 *   │                        Slab Header                         │
 *   ├────────────────────────────────────────────────────────────┤
 *   │  magic      (8 bytes)  - 0x51AB51AB51AB51AB validation     │
 *   │  free_head  (8 bytes)  - first free chunk, or nullptr      │
 *   │  size       (8 bytes)  - chunk size for this slab          │
 *   │  next_slab  (8 bytes)  - next slab in the linked list      │
 *   ├────────────────────────────────────────────────────────────┤
 *   │                         Chunks                             │
 *   ├────────┬────────┬────────┬────────┬────────┬───────────────┤
 *   │ chunk0 │ chunk1 │ chunk2 │ chunk3 │  ...   │    chunkN     │
 *   │ (used) │ (free) │ (free) │ (used) │        │    (free)     │
 *   └────────┴───┬────┴───┬────┴────────┴────────┴───────┬───────┘
 *                │        │                              │
 *                └───→────┴──────────→──────────→────────┴──→ nullptr
 *                       (embedded free list)
 *
 * Free List (within a slab):
 *   - free_head points to first available chunk
 *   - Each free chunk's first 8 bytes store pointer to next free chunk
 *   - Last free chunk points to nullptr
 *   - Allocated chunks have no list pointer (user data overwrites it)
 *
 * Chunk count = (4096 - sizeof(Slab)) / chunk_size
 *
 * Example (32-byte chunks):
 *   Header:  ~32 bytes
 *   Chunks:  (4096 - 32) / 32 = 127 chunks per slab
 */

namespace slab {
    static constexpr std::uint64_t SLAB_MAGIC = 0x51AB'51AB'51AB'51AB; // 51AB == slab in leetspeak

    static constexpr std::size_t SIZE_32 = 32;
    static constexpr std::size_t SIZE_64 = 64;
    static constexpr std::size_t SIZE_128 = 128;
    static constexpr std::size_t SIZE_256 = 256;
    static constexpr std::size_t SIZE_512 = 512;
    static constexpr std::size_t SIZE_1024 = 1024;
    
    static SizeClass classes[] = {
        {SIZE_32, nullptr},
        {SIZE_64, nullptr},
        {SIZE_128, nullptr},
        {SIZE_256, nullptr},
        {SIZE_512, nullptr},
        {SIZE_1024, nullptr}
    };

    // Slabs always exist on a 4K page boundary, so for any aribtrary
    // address we can get the address of its page by masking it to
    // the 4K page boundary
    Slab* addr_to_slab(void* addr) {
        if (addr == nullptr) {
            return nullptr;
        }
        
        static constexpr std::uintptr_t PAGE_ALIGN_MASK = ~0xFFF;
        auto* page = (void*)((std::uintptr_t)addr & PAGE_ALIGN_MASK); // page align
        auto* slab = reinterpret_cast<Slab*>(page);

        if (slab->magic == SLAB_MAGIC) {
            return slab;
        }

        return nullptr;
    }

    bool is_slab(void* addr) {
        return addr_to_slab(addr) != nullptr;
    }

    bool can_alloc(std::size_t size) {
        return size <= 1024;
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

        // instead of storing slab metadata externally, we will just
        // store it directly in the page we just allocated
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

        // Insert this new slab at the front of the linked list:
        // SizeClass -> {this slab} -> {older slab} -> ... -> {oldest slab} -> nullptr
        slab->next_slab = sc->first_slab;
        sc->first_slab = slab;

        return slab;
    }

    void* alloc(std::size_t size) {
        SizeClass* sc = get_size_class(size);
        Slab* slab = sc->first_slab;

        while (slab != nullptr && slab->free_head == nullptr) {
            slab = slab->next_slab;
        }

        if (slab == nullptr) {
            slab = create_slab(sc);
        }

        void* chunk = slab->free_head;

        slab->free_head = *(void**)slab->free_head;

        return chunk;
    }

    void free(void* addr) {
        Slab* slab = addr_to_slab(addr);

        if (slab) {
            *(void**)addr = slab->free_head;
            slab->free_head = addr;
        }
    }
}
