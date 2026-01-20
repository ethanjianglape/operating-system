#ifdef KERNEL_TESTS

#include <test/test.hpp>
#include <containers/kstring.hpp>
#include <log/log.hpp>

namespace test_kstring {
    void test_default_constructor() {
        kstring s;
        test::assert_true(s.empty(), "default constructed kstring is empty");
        test::assert_eq(s.length(), 0ul, "default constructed kstring has length 0");
    }

    void test_cstring_constructor() {
        kstring s("hello");
        test::assert_eq(s.length(), 5ul, "c-string constructor sets correct length");
        test::assert_true(s == "hello", "c-string constructor copies content");
    }

    void test_cstring_constructor_null() {
        kstring s(nullptr);
        test::assert_true(s.empty(), "nullptr constructor creates empty string");
    }

    void test_cstring_count_constructor() {
        kstring s("hello world", 5);
        test::assert_eq(s.length(), 5ul, "count constructor sets correct length");
        test::assert_true(s == "hello", "count constructor copies partial content");
    }

    void test_c_str() {
        kstring s("test");
        test::assert_true(strcmp(s.c_str(), "test") == 0, "c_str() returns correct string");
    }

    void test_c_str_empty() {
        kstring s;
        test::assert_true(strcmp(s.c_str(), "") == 0, "c_str() on empty returns empty string");
    }

    void test_push_back() {
        kstring s("abc");
        s.push_back('d');
        test::assert_eq(s.length(), 4ul, "push_back increases length");
        test::assert_true(s == "abcd", "push_back appends character");
    }

    void test_pop_back() {
        kstring s("abc");
        s.pop_back();
        test::assert_eq(s.length(), 2ul, "pop_back decreases length");
        test::assert_true(s == "ab", "pop_back removes last character");
    }

    void test_pop_back_empty() {
        kstring s;
        s.pop_back();  // Should not crash
        test::assert_true(s.empty(), "pop_back on empty string is safe");
    }

    void test_clear() {
        kstring s("hello");
        s.clear();
        test::assert_true(s.empty(), "clear() empties string");
        test::assert_eq(s.length(), 0ul, "clear() sets length to 0");
    }

    void test_front_back() {
        kstring s("abc");
        test::assert_eq(s.front(), 'a', "front() returns first character");
        test::assert_eq(s.back(), 'c', "back() returns last character");
    }

    void test_operator_bracket() {
        kstring s("hello");
        test::assert_eq(s[1], 'e', "operator[] reads correctly");
        s[1] = 'a';
        test::assert_eq(s[1], 'a', "operator[] writes correctly");
    }

    void test_concatenation_kstring() {
        kstring s1("hello");
        kstring s2(" world");
        s1 += s2;
        test::assert_true(s1 == "hello world", "+= kstring concatenates");
    }

    void test_concatenation_cstring() {
        kstring s("hello");
        s += " world";
        test::assert_true(s == "hello world", "+= c-string concatenates");
    }

    void test_concatenation_char() {
        kstring s("abc");
        s += 'd';
        test::assert_true(s == "abcd", "+= char appends");
    }

    void test_plus_operator() {
        kstring s1("hello");
        kstring s2(" world");
        kstring s3 = s1 + s2;
        test::assert_true(s3 == "hello world", "+ operator concatenates");
        test::assert_true(s1 == "hello", "+ operator doesn't modify original");
    }

    void test_equality_kstring() {
        kstring s1("test");
        kstring s2("test");
        kstring s3("other");
        test::assert_true(s1 == s2, "== returns true for equal strings");
        test::assert_true(s1 != s3, "!= returns true for different strings");
    }

    void test_equality_cstring() {
        kstring s("test");
        test::assert_true(s == "test", "== c-string returns true for equal");
        test::assert_true(s != "other", "!= c-string returns true for different");
    }

    void test_copy_constructor() {
        kstring s1("hello");
        kstring s2(s1);
        test::assert_true(s2 == "hello", "copy constructor copies content");
        s2[0] = 'j';
        test::assert_true(s1 == "hello", "copy constructor creates independent copy");
    }

    void test_move_constructor() {
        kstring s1("hello");
        kstring s2(static_cast<kstring&&>(s1));
        test::assert_true(s2 == "hello", "move constructor transfers content");
        test::assert_true(s1.empty(), "move constructor empties source");
    }

    void test_copy_assignment() {
        kstring s1("hello");
        kstring s2;
        s2 = s1;
        test::assert_true(s2 == "hello", "copy assignment copies content");
    }

    void test_move_assignment() {
        kstring s1("hello");
        kstring s2;
        s2 = static_cast<kstring&&>(s1);
        test::assert_true(s2 == "hello", "move assignment transfers content");
        test::assert_true(s1.empty(), "move assignment empties source");
    }

    void test_cstring_assignment() {
        kstring s;
        s = "hello";
        test::assert_true(s == "hello", "c-string assignment works");
    }

    void test_reverse() {
        kstring s("hello");
        s.reverse();
        test::assert_true(s == "olleh", "reverse() reverses string");
    }

    void test_reverse_empty() {
        kstring s;
        s.reverse();  // Should not crash
        test::assert_true(s.empty(), "reverse() on empty string is safe");
    }

    void test_starts_with() {
        kstring s("hello world");
        kstring prefix("hello");
        kstring other("world");
        test::assert_true(s.starts_with(prefix), "starts_with returns true for prefix");
        test::assert_true(!s.starts_with(other), "starts_with returns false for non-prefix");
    }

    void test_ends_with() {
        kstring s("hello");
        test::assert_true(s.ends_with('o'), "ends_with returns true for matching char");
        test::assert_true(!s.ends_with('x'), "ends_with returns false for non-matching");
    }

    void test_insert() {
        kstring s("hllo");
        s.insert(1, 'e');
        test::assert_true(s == "hello", "insert() inserts character at position");
    }

    void test_erase() {
        kstring s("hello");
        s.erase(1);
        test::assert_true(s == "hllo", "erase() removes character at position");
    }

    void test_truncate() {
        kstring s("hello world");
        s.truncate(5);
        test::assert_true(s == "hello", "truncate() truncates to position");
    }

    void test_iterator() {
        kstring s("abc");
        int sum = 0;
        for (char c : s) {
            sum += c;
        }
        test::assert_eq(sum, 'a' + 'b' + 'c', "range-based for loop works");
    }

    void test_substr_from_start() {
        kstring s("hello world");
        kstring sub = s.substr(0, 5);
        test::assert_true(sub == "hello", "substr(0, 5) returns first 5 chars");
    }

    void test_substr_from_middle() {
        kstring s("hello world");
        kstring sub = s.substr(6, 5);
        test::assert_true(sub == "world", "substr(6, 5) returns 'world'");
    }

    void test_substr_to_end() {
        kstring s("hello world");
        kstring sub = s.substr(6);
        test::assert_true(sub == "world", "substr(6) returns rest of string");
    }

    void test_substr_single_char() {
        kstring s("hello");
        kstring sub = s.substr(1, 1);
        test::assert_true(sub == "e", "substr(1, 1) returns single char");
    }

    void test_substr_pos_out_of_bounds() {
        kstring s("hello");
        kstring sub = s.substr(10, 3);
        test::assert_true(sub.empty(), "substr with pos >= length returns empty");
    }

    void test_substr_len_exceeds_string() {
        kstring s("hello");
        kstring sub = s.substr(2, 100);
        test::assert_true(sub == "llo", "substr with len past end returns to end");
    }

    void test_substr_empty_string() {
        kstring s;
        kstring sub = s.substr(0, 5);
        test::assert_true(sub.empty(), "substr on empty string returns empty");
    }

    void test_large_string_uses_vmm() {
        // Build a string > 1024 bytes to ensure VMM allocation path
        kstring s;
        for (int i = 0; i < 2000; i++) {
            s.push_back('a' + (i % 26));
        }

        test::assert_eq(s.length(), 2000ul, "large string has correct length");
        test::assert_eq(s[0], 'a', "large string first char correct");
        test::assert_eq(s[1999], (char)('a' + (1999 % 26)), "large string last char correct");

        // Test operations on large string
        s.pop_back();
        test::assert_eq(s.length(), 1999ul, "pop_back works on large string");

        kstring copy = s;
        test::assert_eq(copy.length(), 1999ul, "copy of large string has correct length");
    }

    void test_size() {
        kstring s("hello");
        test::assert_eq(s.size(), 5ul, "size() returns correct value");
        test::assert_eq(s.size(), s.length(), "size() equals length()");
    }

    void test_size_empty() {
        kstring s;
        test::assert_eq(s.size(), 0ul, "size() on empty string returns 0");
    }

    void test_reserve() {
        kstring s;
        s.reserve(1000);
        // After reserve, pushing chars shouldn't cause reallocation
        for (int i = 0; i < 100; i++) {
            s.push_back('x');
        }
        test::assert_eq(s.length(), 100ul, "reserve() allows push_back without issue");
    }

    void test_data() {
        kstring s("hello");
        char* ptr = s.data();
        test::assert_eq(ptr[0], 'h', "data() returns pointer to first char");
        test::assert_eq(ptr[4], 'o', "data() allows access to chars");
        ptr[0] = 'j';
        test::assert_true(s == "jello", "data() allows modification");
    }

    void test_const_data() {
        const kstring s("world");
        const char* ptr = s.data();
        test::assert_eq(ptr[0], 'w', "const data() returns pointer");
        test::assert_eq(ptr[4], 'd', "const data() allows read access");
    }

    void test_free_function_plus_operator() {
        kstring s("world");
        kstring result = "hello " + s;
        test::assert_true(result == "hello world", "const char* + kstring concatenates");
    }

    void test_npos() {
        test::assert_eq(kstring::npos, static_cast<std::size_t>(-1), "npos is -1 cast to size_t");
    }

    void test_const_iterator() {
        const kstring s("abc");
        int sum = 0;
        for (auto it = s.begin(); it != s.end(); ++it) {
            sum += *it;
        }
        test::assert_eq(sum, 'a' + 'b' + 'c', "const_iterator works for iteration");
    }

    void test_const_iterator_arithmetic() {
        const kstring s("hello");
        auto it = s.begin();
        auto it2 = it + 2;
        test::assert_eq(*it2, 'l', "const_iterator + n works");
        auto it3 = it2 - 1;
        test::assert_eq(*it3, 'e', "const_iterator - n works");
    }

    void test_const_iterator_difference() {
        const kstring s("hello");
        auto begin = s.begin();
        auto end = s.end();
        auto diff = end - begin;
        test::assert_eq(diff, static_cast<std::ptrdiff_t>(5), "const_iterator difference works");
    }

    void run() {
        log::info("Running kstring tests...");

        test_default_constructor();
        test_cstring_constructor();
        test_cstring_constructor_null();
        test_cstring_count_constructor();
        test_c_str();
        test_c_str_empty();
        test_push_back();
        test_pop_back();
        test_pop_back_empty();
        test_clear();
        test_front_back();
        test_operator_bracket();
        test_concatenation_kstring();
        test_concatenation_cstring();
        test_concatenation_char();
        test_plus_operator();
        test_equality_kstring();
        test_equality_cstring();
        test_copy_constructor();
        test_move_constructor();
        test_copy_assignment();
        test_move_assignment();
        test_cstring_assignment();
        test_reverse();
        test_reverse_empty();
        test_starts_with();
        test_ends_with();
        test_insert();
        test_erase();
        test_truncate();
        test_iterator();
        test_substr_from_start();
        test_substr_from_middle();
        test_substr_to_end();
        test_substr_single_char();
        test_substr_pos_out_of_bounds();
        test_substr_len_exceeds_string();
        test_substr_empty_string();
        test_large_string_uses_vmm();
        test_size();
        test_size_empty();
        test_reserve();
        test_data();
        test_const_data();
        test_free_function_plus_operator();
        test_npos();
        test_const_iterator();
        test_const_iterator_arithmetic();
        test_const_iterator_difference();
    }
}

#endif // KERNEL_TESTS
