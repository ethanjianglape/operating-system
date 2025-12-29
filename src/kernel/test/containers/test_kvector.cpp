#ifdef KERNEL_TESTS

#include <test/test.hpp>
#include <containers/kvector.hpp>
#include <log/log.hpp>

namespace test_kvector {
    void test_default_constructor() {
        kvector<int> v;
        test::assert_true(v.empty(), "default constructed kvector is empty");
        test::assert_eq(v.size(), 0ul, "default constructed kvector has size 0");
    }

    void test_initializer_list_constructor() {
        kvector<int> v = {1, 2, 3, 4, 5};
        test::assert_eq(v.size(), 5ul, "initializer list kvector has correct size");
        test::assert_eq(v[0], 1, "initializer list kvector[0] is correct");
        test::assert_eq(v[4], 5, "initializer list kvector[4] is correct");
    }

    void test_count_constructor() {
        kvector<int> v(5, 42);
        test::assert_eq(v.size(), 5ul, "count constructor creates correct size");
        test::assert_eq(v[0], 42, "count constructor fills with value");
        test::assert_eq(v[4], 42, "count constructor fills all elements");
    }

    void test_push_back() {
        kvector<int> v;
        v.push_back(10);
        v.push_back(20);
        v.push_back(30);
        test::assert_eq(v.size(), 3ul, "push_back increases size");
        test::assert_eq(v[0], 10, "push_back stores first value");
        test::assert_eq(v[2], 30, "push_back stores last value");
    }

    void test_pop_back() {
        kvector<int> v = {1, 2, 3};
        v.pop_back();
        test::assert_eq(v.size(), 2ul, "pop_back decreases size");
        test::assert_eq(v.back(), 2, "pop_back removes last element");
    }

    void test_pop_back_empty() {
        kvector<int> v;
        v.pop_back();  // Should not crash
        test::assert_true(v.empty(), "pop_back on empty vector is safe");
    }

    void test_front_back() {
        kvector<int> v = {10, 20, 30};
        test::assert_eq(v.front(), 10, "front() returns first element");
        test::assert_eq(v.back(), 30, "back() returns last element");
    }

    void test_operator_bracket() {
        kvector<int> v = {5, 10, 15};
        test::assert_eq(v[1], 10, "operator[] reads correctly");
        v[1] = 100;
        test::assert_eq(v[1], 100, "operator[] writes correctly");
    }

    void test_clear() {
        kvector<int> v = {1, 2, 3, 4, 5};
        v.clear();
        test::assert_true(v.empty(), "clear() empties vector");
        test::assert_eq(v.size(), 0ul, "clear() sets size to 0");
    }

    void test_iterator() {
        kvector<int> v = {1, 2, 3};
        int sum = 0;
        for (auto& val : v) {
            sum += val;
        }
        test::assert_eq(sum, 6, "range-based for loop works");
    }

    void test_copy_constructor() {
        kvector<int> v1 = {1, 2, 3};
        kvector<int> v2(v1);
        test::assert_eq(v2.size(), 3ul, "copy constructor copies size");
        test::assert_eq(v2[0], 1, "copy constructor copies elements");
        v2[0] = 100;
        test::assert_eq(v1[0], 1, "copy constructor creates independent copy");
    }

    void test_move_constructor() {
        kvector<int> v1 = {1, 2, 3};
        kvector<int> v2(static_cast<kvector<int>&&>(v1));
        test::assert_eq(v2.size(), 3ul, "move constructor transfers size");
        test::assert_eq(v2[0], 1, "move constructor transfers elements");
        test::assert_true(v1.empty(), "move constructor empties source");
    }

    void test_copy_assignment() {
        kvector<int> v1 = {1, 2, 3};
        kvector<int> v2;
        v2 = v1;
        test::assert_eq(v2.size(), 3ul, "copy assignment copies size");
        test::assert_eq(v2[1], 2, "copy assignment copies elements");
    }

    void test_move_assignment() {
        kvector<int> v1 = {1, 2, 3};
        kvector<int> v2;
        v2 = static_cast<kvector<int>&&>(v1);
        test::assert_eq(v2.size(), 3ul, "move assignment transfers size");
        test::assert_true(v1.empty(), "move assignment empties source");
    }

    void test_emplace_back() {
        kvector<int> v;
        v.emplace_back(42);
        test::assert_eq(v.size(), 1ul, "emplace_back increases size");
        test::assert_eq(v[0], 42, "emplace_back constructs element");
    }

    void test_reserve_and_grow() {
        kvector<int> v;
        v.reserve(100);
        for (int i = 0; i < 100; i++) {
            v.push_back(i);
        }
        test::assert_eq(v.size(), 100ul, "vector grows to hold 100 elements");
        test::assert_eq(v[99], 99, "vector stores all elements correctly");
    }

    void test_initializer_list_assignment() {
        kvector<int> v = {1, 2, 3};
        v = {10, 20};
        test::assert_eq(v.size(), 2ul, "initializer list assignment updates size");
        test::assert_eq(v[0], 10, "initializer list assignment updates elements");
    }

    void test_large_allocation_uses_vmm() {
        // Each element is 512 bytes, 4 elements = 2048 bytes > 1024, uses VMM
        struct LargeStruct {
            std::uint8_t data[512];
        };

        kvector<LargeStruct> v;
        for (int i = 0; i < 4; i++) {
            LargeStruct s{};
            s.data[0] = static_cast<std::uint8_t>(i);
            s.data[511] = static_cast<std::uint8_t>(i * 2);
            v.push_back(s);
        }

        test::assert_eq(v.size(), 4ul, "large struct vector has correct size");
        test::assert_eq(v[0].data[0], (std::uint8_t)0, "large struct vector stores first element");
        test::assert_eq(v[3].data[511], (std::uint8_t)6, "large struct vector stores last element");
    }

    void run() {
        log::info("Running kvector tests...");

        test_default_constructor();
        test_initializer_list_constructor();
        test_count_constructor();
        test_push_back();
        test_pop_back();
        test_pop_back_empty();
        test_front_back();
        test_operator_bracket();
        test_clear();
        test_iterator();
        test_copy_constructor();
        test_move_constructor();
        test_copy_assignment();
        test_move_assignment();
        test_emplace_back();
        test_reserve_and_grow();
        test_initializer_list_assignment();
        test_large_allocation_uses_vmm();
    }
}

#endif // KERNEL_TESTS
