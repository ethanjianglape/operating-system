#ifdef KERNEL_TESTS

#include <test/test.hpp>
#include <containers/klist.hpp>
#include <containers/kstring.hpp>
#include <log/log.hpp>

namespace test_klist {
    void test_default_constructor() {
        klist<int> l;
        test::assert_true(l.empty(), "default constructed klist is empty");
        test::assert_eq(l.size(), 0ul, "default constructed klist has size 0");
    }

    void test_count_constructor() {
        klist<int> l(5, 42);
        test::assert_eq(l.size(), 5ul, "count constructor creates correct size");
        test::assert_eq(l[0], 42, "count constructor fills with value");
        test::assert_eq(l[4], 42, "count constructor fills all elements");
    }

    void test_push_back() {
        klist<int> l;
        l.push_back(10);
        l.push_back(20);
        l.push_back(30);
        test::assert_eq(l.size(), 3ul, "push_back increases size");
        test::assert_eq(l[0], 10, "push_back stores first value");
        test::assert_eq(l[2], 30, "push_back stores last value");
    }

    void test_push_front() {
        klist<int> l;
        l.push_front(10);
        l.push_front(20);
        l.push_front(30);
        test::assert_eq(l.size(), 3ul, "push_front increases size");
        test::assert_eq(l[0], 30, "push_front inserts at front");
        test::assert_eq(l[2], 10, "push_front pushes existing to back");
    }

    void test_pop_back() {
        klist<int> l;
        l.push_back(1);
        l.push_back(2);
        l.push_back(3);
        l.pop_back();
        test::assert_eq(l.size(), 2ul, "pop_back decreases size");
        test::assert_eq(l.back(), 2, "pop_back removes last element");
    }

    void test_pop_front() {
        klist<int> l;
        l.push_back(1);
        l.push_back(2);
        l.push_back(3);
        l.pop_front();
        test::assert_eq(l.size(), 2ul, "pop_front decreases size");
        test::assert_eq(l.front(), 2, "pop_front removes first element");
    }

    void test_pop_back_empty() {
        klist<int> l;
        l.pop_back();  // Should not crash
        test::assert_true(l.empty(), "pop_back on empty list is safe");
    }

    void test_pop_front_empty() {
        klist<int> l;
        l.pop_front();  // Should not crash
        test::assert_true(l.empty(), "pop_front on empty list is safe");
    }

    void test_pop_back_to_empty() {
        klist<int> l;
        l.push_back(42);
        l.pop_back();
        test::assert_true(l.empty(), "pop_back single element empties list");
        test::assert_eq(l.size(), 0ul, "pop_back single element sets size to 0");
    }

    void test_pop_front_to_empty() {
        klist<int> l;
        l.push_front(42);
        l.pop_front();
        test::assert_true(l.empty(), "pop_front single element empties list");
        test::assert_eq(l.size(), 0ul, "pop_front single element sets size to 0");
    }

    void test_front_back() {
        klist<int> l;
        l.push_back(10);
        l.push_back(20);
        l.push_back(30);
        test::assert_eq(l.front(), 10, "front() returns first element");
        test::assert_eq(l.back(), 30, "back() returns last element");
    }

    void test_front_back_single() {
        klist<int> l;
        l.push_back(42);
        test::assert_eq(l.front(), 42, "front() on single element");
        test::assert_eq(l.back(), 42, "back() on single element (same as front)");
    }

    void test_operator_bracket() {
        klist<int> l;
        l.push_back(5);
        l.push_back(10);
        l.push_back(15);
        test::assert_eq(l[1], 10, "operator[] reads correctly");
        l[1] = 100;
        test::assert_eq(l[1], 100, "operator[] writes correctly");
    }

    void test_clear() {
        klist<int> l;
        l.push_back(1);
        l.push_back(2);
        l.push_back(3);
        l.push_back(4);
        l.push_back(5);
        l.clear();
        test::assert_true(l.empty(), "clear() empties list");
        test::assert_eq(l.size(), 0ul, "clear() sets size to 0");
    }

    void test_copy_constructor() {
        klist<int> l1;
        l1.push_back(1);
        l1.push_back(2);
        l1.push_back(3);
        klist<int> l2(l1);
        test::assert_eq(l2.size(), 3ul, "copy constructor copies size");
        test::assert_eq(l2[0], 1, "copy constructor copies elements");
        l2[0] = 100;
        test::assert_eq(l1[0], 1, "copy constructor creates independent copy");
    }

    void test_copy_constructor_empty() {
        klist<int> l1;
        klist<int> l2(l1);
        test::assert_true(l2.empty(), "copy constructor of empty list is empty");
    }

    void test_move_constructor() {
        klist<int> l1;
        l1.push_back(1);
        l1.push_back(2);
        l1.push_back(3);
        klist<int> l2(static_cast<klist<int>&&>(l1));
        test::assert_eq(l2.size(), 3ul, "move constructor transfers size");
        test::assert_eq(l2[0], 1, "move constructor transfers elements");
        test::assert_true(l1.empty(), "move constructor empties source");
    }

    void test_copy_assignment() {
        klist<int> l1;
        l1.push_back(1);
        l1.push_back(2);
        l1.push_back(3);
        klist<int> l2;
        l2 = l1;
        test::assert_eq(l2.size(), 3ul, "copy assignment copies size");
        test::assert_eq(l2[1], 2, "copy assignment copies elements");
        l2[0] = 100;
        test::assert_eq(l1[0], 1, "copy assignment creates independent copy");
    }

    void test_move_assignment() {
        klist<int> l1;
        l1.push_back(1);
        l1.push_back(2);
        l1.push_back(3);
        klist<int> l2;
        l2 = static_cast<klist<int>&&>(l1);
        test::assert_eq(l2.size(), 3ul, "move assignment transfers size");
        test::assert_eq(l2[0], 1, "move assignment transfers elements");
        test::assert_true(l1.empty(), "move assignment empties source");
    }

    void test_circular_structure() {
        // Verify the circular nature by checking front/back after various operations
        klist<int> l;
        l.push_back(1);
        l.push_back(2);
        l.push_back(3);

        // After push_back: 1 <-> 2 <-> 3 <-> (back to 1)
        test::assert_eq(l.front(), 1, "circular: front is 1");
        test::assert_eq(l.back(), 3, "circular: back is 3");

        l.push_front(0);
        // After push_front: 0 <-> 1 <-> 2 <-> 3 <-> (back to 0)
        test::assert_eq(l.front(), 0, "circular: front is 0 after push_front");
        test::assert_eq(l.back(), 3, "circular: back still 3 after push_front");

        l.pop_back();
        // After pop_back: 0 <-> 1 <-> 2 <-> (back to 0)
        test::assert_eq(l.back(), 2, "circular: back is 2 after pop_back");

        l.pop_front();
        // After pop_front: 1 <-> 2 <-> (back to 1)
        test::assert_eq(l.front(), 1, "circular: front is 1 after pop_front");
    }

    void test_many_elements() {
        klist<int> l;

        for (int i = 0; i < 100; i++) {
            l.push_back(i);
        }

        test::assert_eq(l.size(), 100ul, "many elements: size is 100");
        test::assert_eq(l.front(), 0, "many elements: front is 0");
        test::assert_eq(l.back(), 99, "many elements: back is 99");

        bool all_correct = true;
        for (int i = 0; i < 100 && all_correct; i++) {
            all_correct = (l[i] == i);
        }
        test::assert_true(all_correct, "many elements: all values correct via operator[]");
    }

    void test_alternating_push() {
        klist<int> l;

        // Alternate push_front and push_back
        l.push_back(0);   // [0]
        l.push_front(1);  // [1, 0]
        l.push_back(2);   // [1, 0, 2]
        l.push_front(3);  // [3, 1, 0, 2]
        l.push_back(4);   // [3, 1, 0, 2, 4]

        test::assert_eq(l.size(), 5ul, "alternating push: size is 5");
        test::assert_eq(l[0], 3, "alternating push: [0] is 3");
        test::assert_eq(l[1], 1, "alternating push: [1] is 1");
        test::assert_eq(l[2], 0, "alternating push: [2] is 0");
        test::assert_eq(l[3], 2, "alternating push: [3] is 2");
        test::assert_eq(l[4], 4, "alternating push: [4] is 4");
    }

    void test_alternating_pop() {
        klist<int> l;
        for (int i = 0; i < 6; i++) {
            l.push_back(i);
        }
        // [0, 1, 2, 3, 4, 5]

        l.pop_front();  // [1, 2, 3, 4, 5]
        l.pop_back();   // [1, 2, 3, 4]
        l.pop_front();  // [2, 3, 4]

        test::assert_eq(l.size(), 3ul, "alternating pop: size is 3");
        test::assert_eq(l.front(), 2, "alternating pop: front is 2");
        test::assert_eq(l.back(), 4, "alternating pop: back is 4");
    }

    void test_reuse_after_clear() {
        klist<int> l;
        l.push_back(1);
        l.push_back(2);
        l.clear();

        l.push_back(10);
        l.push_back(20);

        test::assert_eq(l.size(), 2ul, "reuse after clear: size is 2");
        test::assert_eq(l[0], 10, "reuse after clear: [0] is 10");
        test::assert_eq(l[1], 20, "reuse after clear: [1] is 20");
    }

    void test_kstring_elements() {
        klist<kstring> l;
        l.push_back(kstring("hello"));
        l.push_back(kstring("world"));
        l.push_back(kstring("test"));

        test::assert_eq(l.size(), 3ul, "kstring: size is 3");
        test::assert_true(l[0] == "hello", "kstring: [0] is hello");
        test::assert_true(l[2] == "test", "kstring: [2] is test");

        l.pop_back();
        test::assert_eq(l.size(), 2ul, "kstring: size after pop_back is 2");

        l.clear();
        test::assert_true(l.empty(), "kstring: clear empties list");
    }

    void test_kstring_copy() {
        klist<kstring> l1;
        l1.push_back(kstring("first"));
        l1.push_back(kstring("second"));

        klist<kstring> l2(l1);

        test::assert_eq(l2.size(), 2ul, "kstring copy: size is 2");
        test::assert_true(l2[0] == "first", "kstring copy: [0] is first");

        // Modify original, verify copy is independent
        l1[0] = kstring("modified");
        test::assert_true(l2[0] == "first", "kstring copy: independent of original");
    }

    void run() {
        log::info("Running klist tests...");

        test_default_constructor();
        test_count_constructor();
        test_push_back();
        test_push_front();
        test_pop_back();
        test_pop_front();
        test_pop_back_empty();
        test_pop_front_empty();
        test_pop_back_to_empty();
        test_pop_front_to_empty();
        test_front_back();
        test_front_back_single();
        test_operator_bracket();
        test_clear();
        test_copy_constructor();
        test_copy_constructor_empty();
        test_move_constructor();
        test_copy_assignment();
        test_move_assignment();
        test_circular_structure();
        test_many_elements();
        test_alternating_push();
        test_alternating_pop();
        test_reuse_after_clear();
        test_kstring_elements();
        test_kstring_copy();
    }
}

#endif // KERNEL_TESTS
