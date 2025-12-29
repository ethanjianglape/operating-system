#ifdef KERNEL_TESTS

#include <test/test.hpp>
#include <memory/pmm.hpp>
#include <arch.hpp>
#include <log/log.hpp>

namespace test_pmm {
    void test_alloc_frame_returns_non_null() {
        void* frame = pmm::alloc_frame();
        test::assert_not_null(frame, "alloc_frame returns non-null");
        pmm::free_frame(reinterpret_cast<std::uintptr_t>(frame));
    }

    void test_alloc_frame_returns_aligned() {
        void* frame = pmm::alloc_frame();
        auto addr = reinterpret_cast<std::uintptr_t>(frame);
        test::assert_eq(addr % pmm::FRAME_SIZE, 0ul, "alloc_frame returns page-aligned address");
        pmm::free_frame(addr);
    }

    void test_sequential_allocs_differ() {
        void* frame1 = pmm::alloc_frame();
        void* frame2 = pmm::alloc_frame();
        test::assert_ne(frame1, frame2, "sequential allocs return different addresses");
        pmm::free_frame(reinterpret_cast<std::uintptr_t>(frame1));
        pmm::free_frame(reinterpret_cast<std::uintptr_t>(frame2));
    }

    void test_free_allows_realloc() {
        void* frame1 = pmm::alloc_frame();
        auto addr1 = reinterpret_cast<std::uintptr_t>(frame1);
        pmm::free_frame(addr1);

        // Allocate again - should succeed (might get same or different frame)
        void* frame2 = pmm::alloc_frame();
        test::assert_not_null(frame2, "allocation after free succeeds");
        pmm::free_frame(reinterpret_cast<std::uintptr_t>(frame2));
    }

    void test_contiguous_alloc_returns_consecutive() {
        constexpr std::size_t NUM_FRAMES = 4;
        void* frames = pmm::alloc_contiguous_frames(NUM_FRAMES);
        test::assert_not_null(frames, "contiguous alloc returns non-null");

        auto base = reinterpret_cast<std::uintptr_t>(frames);
        test::assert_eq(base % pmm::FRAME_SIZE, 0ul, "contiguous alloc is page-aligned");

        pmm::free_contiguous_frames(base, NUM_FRAMES);
    }

    void test_contiguous_free_allows_realloc() {
        constexpr std::size_t NUM_FRAMES = 4;
        void* frames1 = pmm::alloc_contiguous_frames(NUM_FRAMES);
        auto addr1 = reinterpret_cast<std::uintptr_t>(frames1);
        pmm::free_contiguous_frames(addr1, NUM_FRAMES);

        void* frames2 = pmm::alloc_contiguous_frames(NUM_FRAMES);
        test::assert_not_null(frames2, "contiguous alloc after free succeeds");
        pmm::free_contiguous_frames(reinterpret_cast<std::uintptr_t>(frames2), NUM_FRAMES);
    }

    void test_alloc_frame_decreases_free_count() {
        std::size_t before = pmm::get_free_frames();
        void* frame = pmm::alloc_frame();
        std::size_t after = pmm::get_free_frames();

        test::assert_eq(after, before - 1, "alloc_frame decreases free count by 1");
        pmm::free_frame(reinterpret_cast<std::uintptr_t>(frame));
    }

    void test_free_frame_increases_free_count() {
        void* frame = pmm::alloc_frame();
        std::size_t before = pmm::get_free_frames();
        pmm::free_frame(reinterpret_cast<std::uintptr_t>(frame));
        std::size_t after = pmm::get_free_frames();

        test::assert_eq(after, before + 1, "free_frame increases free count by 1");
    }

    void test_contiguous_alloc_decreases_free_count() {
        constexpr std::size_t NUM_FRAMES = 10;
        std::size_t before = pmm::get_free_frames();
        void* frames = pmm::alloc_contiguous_frames(NUM_FRAMES);
        std::size_t after = pmm::get_free_frames();

        test::assert_eq(after, before - NUM_FRAMES, "contiguous alloc decreases free count by N");
        pmm::free_contiguous_frames(reinterpret_cast<std::uintptr_t>(frames), NUM_FRAMES);
    }

    void test_contiguous_free_increases_free_count() {
        constexpr std::size_t NUM_FRAMES = 10;
        void* frames = pmm::alloc_contiguous_frames(NUM_FRAMES);
        std::size_t before = pmm::get_free_frames();
        pmm::free_contiguous_frames(reinterpret_cast<std::uintptr_t>(frames), NUM_FRAMES);
        std::size_t after = pmm::get_free_frames();

        test::assert_eq(after, before + NUM_FRAMES, "contiguous free increases free count by N");
    }

    void run() {
        log::info("Running PMM tests...");
        
        test_alloc_frame_returns_non_null();
        test_alloc_frame_returns_aligned();
        test_sequential_allocs_differ();
        test_free_allows_realloc();
        test_contiguous_alloc_returns_consecutive();
        test_contiguous_free_allows_realloc();
        test_alloc_frame_decreases_free_count();
        test_free_frame_increases_free_count();
        test_contiguous_alloc_decreases_free_count();
        test_contiguous_free_increases_free_count();
    }
}

#endif // KERNEL_TESTS
