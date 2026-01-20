/**
 * @file slab.cpp
 * @brief Slab allocator for fixed-size kernel object allocation.
 *
 * Organization:
 *   - Each SizeClass (32, 64, 128, 256, 512, 1024 bytes) maintains a
 *     doubly-linked list of Slabs
 *   - New slabs are inserted at the head for O(1) insertion
 *   - Allocation searches from the head, so the newest slab (most likely
 *     to have free chunks) is checked first
 *   - Empty slabs are destroyed and returned to the VMM (unless it's the
 *     only slab in the SizeClass)
 *
 *   SizeClass (e.g. 64-byte)
 *       │
 *       ▼
 *   ┌────────┐    ┌───────┐    ┌────────┐
 *   │ Slab   │◀──▶│ Slab  │◀──▶│ Slab   │──▶ nullptr
 *   │(newest)│    │       │    │(oldest)│
 *   └────────┘    └───────┘    └────────┘
 *
 * Slab Page Layout:
 *
 *   A slab is a single 4KB page divided into fixed-size chunks.
 *   The Slab metadata is stored at the start of the page itself.
 *
 *   ┌─────────────────────────────────────────────────────────────────┐
 *   │                          Slab Header                            │
 *   ├─────────────────────────────────────────────────────────────────┤
 *   │  magic            (8 bytes)  - 0x51AB51AB51AB51AB validation    │
 *   │  free_head        (8 bytes)  - first free chunk, or nullptr     │
 *   │  next_slab        (8 bytes)  - next slab in doubly-linked list  │
 *   │  prev_slab        (8 bytes)  - prev slab in doubly-linked list  │
 *   │  size_class_index (1 byte)   - index into classes[] array       │
 *   │  free_chunks      (1 byte)   - number of free chunks            │
 *   │  (padding)        (6 bytes)  - alignment padding                │
 *   ├─────────────────────────────────────────────────────────────────┤
 *   │                            Chunks                               │
 *   ├────────┬────────┬────────┬────────┬────────┬────────────────────┤
 *   │ chunk0 │ chunk1 │ chunk2 │ chunk3 │  ...   │      chunkN        │
 *   │ (used) │ (free) │ (free) │ (used) │        │      (free)        │
 *   └────────┴───┬────┴───┬────┴────────┴────────┴──────────┬─────────┘
 *                │        │                                 │
 *                └───→────┴──────────→──────────→───────────┴──→ nullptr
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
 *   Header:  40 bytes
 *   Chunks:  (4096 - 40) / 32 = 126 chunks per slab
 */

#include <memory/slab.hpp>
#include <log/log.hpp>
#include <arch.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace slab {   
    constexpr std::uint8_t chunks_per_slab(std::size_t chunk_size) {
        return (arch::vmm::PAGE_SIZE - sizeof(Slab)) / chunk_size;
    }

    static SizeClass classes[] = {
        {0, SIZE_32, 0, nullptr, chunks_per_slab(SIZE_32)},
        {1, SIZE_64, 0, nullptr, chunks_per_slab(SIZE_64)},
        {2, SIZE_128, 0, nullptr, chunks_per_slab(SIZE_128)},
        {3, SIZE_256, 0, nullptr, chunks_per_slab(SIZE_256)},
        {4, SIZE_512, 0, nullptr, chunks_per_slab(SIZE_512)},
        {5, SIZE_1024, 0, nullptr, chunks_per_slab(SIZE_1024)}
    };

    /**
     * @brief Gets the Slab containing an address, if it's a valid slab allocation.
     *
     * Slabs always exist on a 4K page boundary, so we mask the address to find
     * the page start, then validate the magic number.
     *
     * @param addr Any address potentially within a slab.
     * @return Pointer to the Slab, or nullptr if not a valid slab address.
     */
    Slab* try_get_slab(void* addr) {
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
        return try_get_slab(addr) != nullptr;
    }

    bool can_alloc(std::size_t bytes) {
        return bytes <= 1024;
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

    /**
     * @brief Creates a new slab for a size class.
     *
     * Allocates a 4K page, initializes the slab header and free list,
     * then inserts the slab at the head of the size class's slab list.
     *
     * @param sc The size class to create a slab for.
     * @return Pointer to the new Slab.
     */
    Slab* create_slab(SizeClass* sc) {
        void* page = arch::vmm::alloc_kpage();
        Slab* slab = reinterpret_cast<Slab*>(page);

        // instead of storing slab metadata externally, we will just
        // store it directly in the page we just allocated
        auto* first_chunk = static_cast<std::uint8_t*>(page) + sizeof(Slab);
        std::uint8_t num_chunks = sc->chunks_per_slab;

        for (std::uint8_t i = 0; i < num_chunks - 1; i++) {
            std::uint8_t* this_chunk = first_chunk + (i * sc->size);
            std::uint8_t* next_chunk = this_chunk + sc->size;

            *(void**)this_chunk = next_chunk;
        }

        std::uint8_t* last_chunk = first_chunk + ((num_chunks - 1) * sc->size);
        *(void**)last_chunk = nullptr;

        slab->magic = SLAB_MAGIC;
        slab->size_class_index = sc->index;
        slab->free_head = first_chunk;
        slab->free_chunks = num_chunks;

        // Insert this new slab at the front of the linked list:
        // SizeClass -> {this slab} <-> {older slab} <-> ... <-> {oldest slab} -> nullptr
        Slab* first_slab = sc->first_slab;

        if (first_slab) {
            first_slab->prev_slab = slab;
        }
        
        slab->next_slab = first_slab;
        slab->prev_slab = nullptr; // The first slab in the list doesn't have a prev slab
        
        sc->first_slab = slab;
        sc->num_slabs += 1;

        return slab;
    }

    /**
     * @brief Destroys a slab and returns its page to the VMM.
     *
     * Unlinks the slab from its size class's doubly-linked list.
     * Refuses to destroy the last slab in a size class.
     *
     * @param sc The size class the slab belongs to.
     * @param slab The slab to destroy.
     */
    void destroy_slab(SizeClass* sc, Slab* slab) {
        if (sc == nullptr || slab == nullptr) {
            log::warn("Attempt to destroy NULL slab");
            return;
        }

        Slab* prev = slab->prev_slab;
        Slab* next = slab->next_slab;

        if (prev == nullptr && next == nullptr) {
            log::warn("Attempt to destroy only slab in SizeClass: ", sc->size);
            return;
        }

        if (prev) {
            prev->next_slab = next;
        } else {
            sc->first_slab = next;
        }

        if (next) {
            next->prev_slab = prev;
        }

        sc->num_slabs -= 1;

        arch::vmm::free_kpage(slab);
    }

    /**
     * @brief Allocates memory from the slab allocator.
     *
     * Finds the appropriate size class, locates a slab with free chunks
     * (creating one if needed), and returns a chunk from the free list.
     *
     * @param size Number of bytes to allocate (max 1024).
     * @return Pointer to the allocated memory.
     */
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
        slab->free_chunks -= 1;

        return chunk;
    }

    /**
     * @brief Frees memory back to the slab allocator.
     *
     * Returns the chunk to its slab's free list. If the slab becomes
     * completely empty and isn't the last slab in its size class, the
     * slab is destroyed.
     *
     * @param addr Pointer previously returned by slab::alloc().
     */
    void free(void* addr) {
        Slab* slab = try_get_slab(addr);

        if (slab == nullptr) {
            return;
        }

        SizeClass* sc = &classes[slab->size_class_index];

        *(void**)addr = slab->free_head;
        slab->free_head = addr;
        slab->free_chunks += 1;

        // If freeing a chunk from this slab results in an empty slab, it is
        // no longer needed, so destroy it, unless this is the only slab in the
        // SizeClass
        if (slab->free_chunks == sc->chunks_per_slab && sc->num_slabs > 1) {
            destroy_slab(sc, slab);
        }
    }

    std::size_t total_slabs() {
        std::size_t total = 0;
        
        for (const auto& sc : classes) {
            total += sc.num_slabs;
        }
        
        return total;
    }
}
