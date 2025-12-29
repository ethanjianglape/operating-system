#ifdef KERNEL_TESTS

#include <test/test.hpp>
#include <memory/memory.hpp>
#include <memory/slab.hpp>
#include <log/log.hpp>

namespace test_kmalloc {
    void test_kmalloc_zero_returns_null() {
        void* ptr = kmalloc(0);
        test::assert_null(ptr, "kmalloc(0) returns nullptr");
    }

    void test_kmalloc_small_returns_non_null() {
        void* ptr = kmalloc(32);
        test::assert_not_null(ptr, "kmalloc(32) returns non-null");
        kfree(ptr);
    }

    void test_kmalloc_large_returns_non_null() {
        void* ptr = kmalloc(2048);
        test::assert_not_null(ptr, "kmalloc(2048) returns non-null");
        kfree(ptr);
    }

    void test_small_alloc_uses_slab() {
        void* ptr = kmalloc(64);
        test::assert_true(slab::is_slab(ptr), "small kmalloc uses slab allocator");
        kfree(ptr);
    }

    void test_large_alloc_does_not_use_slab() {
        void* ptr = kmalloc(2048);
        test::assert_true(!slab::is_slab(ptr), "large kmalloc does not use slab allocator");
        kfree(ptr);
    }

    void test_kfree_null_is_safe() {
        kfree(nullptr);  // Should not crash
        test::assert_true(true, "kfree(nullptr) does not crash");
    }

    void test_sequential_allocs_differ() {
        void* ptr1 = kmalloc(64);
        void* ptr2 = kmalloc(64);
        test::assert_ne(ptr1, ptr2, "sequential kmalloc returns different addresses");
        kfree(ptr1);
        kfree(ptr2);
    }

    void test_free_allows_realloc() {
        void* ptr1 = kmalloc(128);
        kfree(ptr1);

        void* ptr2 = kmalloc(128);
        test::assert_not_null(ptr2, "kmalloc after kfree succeeds");
        kfree(ptr2);
    }

    void test_small_alloc_is_writable() {
        constexpr std::size_t SIZE = 64;
        auto* ptr = static_cast<std::uint8_t*>(kmalloc(SIZE));

        for (std::size_t i = 0; i < SIZE; i++) {
            ptr[i] = static_cast<std::uint8_t>(i);
        }

        bool valid = true;
        for (std::size_t i = 0; i < SIZE; i++) {
            if (ptr[i] != static_cast<std::uint8_t>(i)) {
                valid = false;
                break;
            }
        }

        test::assert_true(valid, "small kmalloc memory is readable/writable");
        kfree(ptr);
    }

    void test_large_alloc_is_writable() {
        constexpr std::size_t SIZE = 2048;
        auto* ptr = static_cast<std::uint8_t*>(kmalloc(SIZE));

        for (std::size_t i = 0; i < SIZE; i++) {
            ptr[i] = static_cast<std::uint8_t>(i & 0xFF);
        }

        bool valid = true;
        for (std::size_t i = 0; i < SIZE; i++) {
            if (ptr[i] != static_cast<std::uint8_t>(i & 0xFF)) {
                valid = false;
                break;
            }
        }

        test::assert_true(valid, "large kmalloc memory is readable/writable");
        kfree(ptr);
    }

    void test_kalloc_template() {
        auto* arr = kalloc<std::uint32_t>(10);
        test::assert_not_null(arr, "kalloc<uint32_t>(10) returns non-null");

        for (int i = 0; i < 10; i++) {
            arr[i] = i * 100;
        }

        bool valid = true;
        for (int i = 0; i < 10; i++) {
            if (arr[i] != static_cast<std::uint32_t>(i * 100)) {
                valid = false;
                break;
            }
        }

        test::assert_true(valid, "kalloc<T> memory is usable as T array");
        kfree(arr);
    }

    void test_boundary_size_uses_slab() {
        void* ptr = kmalloc(1024);  // Exactly at slab boundary
        test::assert_true(slab::is_slab(ptr), "kmalloc(1024) uses slab allocator");
        kfree(ptr);
    }

    void test_above_boundary_uses_vmm() {
        void* ptr = kmalloc(1025);  // Just above slab boundary
        test::assert_true(!slab::is_slab(ptr), "kmalloc(1025) uses VMM allocator");
        kfree(ptr);
    }

    void run() {
        log::info("Running kmalloc tests...");

        test_kmalloc_zero_returns_null();
        test_kmalloc_small_returns_non_null();
        test_kmalloc_large_returns_non_null();
        test_small_alloc_uses_slab();
        test_large_alloc_does_not_use_slab();
        test_kfree_null_is_safe();
        test_sequential_allocs_differ();
        test_free_allows_realloc();
        test_small_alloc_is_writable();
        test_large_alloc_is_writable();
        test_kalloc_template();
        test_boundary_size_uses_slab();
        test_above_boundary_uses_vmm();
    }
}

#endif // KERNEL_TESTS
