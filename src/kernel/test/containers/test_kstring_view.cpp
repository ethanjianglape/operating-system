#ifdef KERNEL_TESTS

#include <containers/kstring_view.hpp>
#include <crt/crt.h>
#include <log/log.hpp>
#include <test/test.hpp>

#include <utility>

namespace test_kstring_view {

void test_cstring_constructor()
{
    kstring_view sv("hello");
    test::assert_eq(sv.length(), 5ul, "cstring constructor sets correct length");
    test::assert_true(strcmp(sv.c_str(), "hello") == 0, "cstring constructor points to correct data");
}

void test_cstring_len_constructor()
{
    kstring_view sv("hello world", 5);
    test::assert_eq(sv.length(), 5ul, "cstring+len constructor sets correct length");
    test::assert_true(strncmp(sv.data(), "hello", 5) == 0, "cstring+len constructor points to correct data");
}

void test_copy_constructor()
{
    kstring_view sv1("hello");
    kstring_view sv2(sv1);
    test::assert_eq(sv2.length(), sv1.length(), "copy constructor preserves length");
    test::assert_true(strcmp(sv2.c_str(), "hello") == 0, "copy constructor preserves data");
}

void test_move_constructor()
{
    kstring_view sv1("hello");
    kstring_view sv2(std::move(sv1));
    test::assert_true(strcmp(sv2.c_str(), "hello") == 0, "move constructor transfers data");
}

void test_empty_false()
{
    kstring_view sv("hello");
    test::assert_true(!sv.empty(), "non-empty string reports not empty");
}

void test_empty_true()
{
    kstring_view sv("");
    test::assert_true(sv.empty(), "empty string reports empty");
}

void test_size_equals_length()
{
    kstring_view sv("hello");
    test::assert_eq(sv.size(), sv.length(), "size() equals length()");
    test::assert_eq(sv.size(), 5ul, "size() returns correct value");
}

void test_front()
{
    kstring_view sv("abc");
    test::assert_eq(sv.front(), 'a', "front() returns first character");
}

void test_back()
{
    kstring_view sv("abc");
    test::assert_eq(sv.back(), 'c', "back() returns last character");
}

void test_operator_bracket()
{
    kstring_view sv("hello");
    test::assert_eq(sv[0], 'h', "operator[] returns correct char at 0");
    test::assert_eq(sv[4], 'o', "operator[] returns correct char at end");
}

void test_c_str()
{
    kstring_view sv("test");
    test::assert_true(strcmp(sv.c_str(), "test") == 0, "c_str() returns correct string");
}

void test_data()
{
    kstring_view sv("hello");
    test::assert_true(sv.data() == sv.c_str(), "data() and c_str() return same pointer");
}

void test_find_present_at_start()
{
    kstring_view sv("hello world");
    test::assert_eq(sv.find("hello"), 0ul, "find() returns 0 for prefix");
}

void test_find_present_in_middle()
{
    kstring_view sv("hello world");
    test::assert_eq(sv.find("world"), 6ul, "find() returns correct position for middle match");
}

void test_find_absent()
{
    kstring_view sv("hello");
    test::assert_eq(sv.find("xyz"), kstring_view::npos, "find() returns npos for absent substring");
}

void test_find_null()
{
    kstring_view sv("hello");
    test::assert_eq(sv.find(nullptr), kstring_view::npos, "find(nullptr) returns npos");
}

void test_find_empty_string()
{
    kstring_view sv("hello");
    test::assert_eq(sv.find(""), kstring_view::npos, "find(\"\") returns npos");
}

void test_find_longer_than_source()
{
    kstring_view sv("hi");
    test::assert_eq(sv.find("hello"), kstring_view::npos, "find() returns npos when target is longer than source");
}

void test_npos()
{
    test::assert_eq(kstring_view::npos, static_cast<std::size_t>(-1), "npos is SIZE_MAX");
}

void test_substr_from_start()
{
    kstring_view sv("hello world");
    kstring_view sub = sv.substr(0, 5);
    test::assert_eq(sub.length(), 5ul, "substr(0, 5) has correct length");
    test::assert_true(strncmp(sub.data(), "hello", 5) == 0, "substr(0, 5) returns correct data");
}

void test_substr_from_middle()
{
    kstring_view sv("hello world");
    kstring_view sub = sv.substr(6, 5);
    test::assert_eq(sub.length(), 5ul, "substr(6, 5) has correct length");
    test::assert_true(strcmp(sub.c_str(), "world") == 0, "substr(6, 5) returns 'world'");
}

void test_substr_to_end()
{
    kstring_view sv("hello world");
    kstring_view sub = sv.substr(6);
    test::assert_true(strcmp(sub.c_str(), "world") == 0, "substr(6) returns rest of string");
}

void test_substr_pos_out_of_bounds()
{
    kstring_view sv("hello");
    kstring_view sub = sv.substr(10);
    test::assert_true(sub.empty(), "substr with pos >= length returns empty");
}

void test_substr_single_char()
{
    kstring_view sv("hello");
    kstring_view sub = sv.substr(1, 1);
    test::assert_eq(sub.length(), 1ul, "substr(1, 1) has length 1");
    test::assert_eq(sub[0], 'e', "substr(1, 1) returns correct char");
}

void test_copy_assignment()
{
    kstring_view sv1("hello");
    kstring_view sv2("world");
    sv2 = sv1;
    test::assert_true(sv2 == "hello", "copy assignment transfers content");
    test::assert_true(sv1 == "hello", "copy assignment does not modify source");
}

void test_move_assignment()
{
    kstring_view sv1("hello");
    kstring_view sv2("world");
    sv2 = std::move(sv1);
    test::assert_true(sv2 == "hello", "move assignment transfers content");
}

void test_equality_equal()
{
    kstring_view sv1("hello");
    kstring_view sv2("hello");
    test::assert_true(sv1 == sv2, "== returns true for views with same content");
}

void test_equality_different_content()
{
    kstring_view sv1("hello");
    kstring_view sv2("world");
    test::assert_true(!(sv1 == sv2), "== returns false for views with different content");
}

void test_equality_different_length()
{
    kstring_view sv1("hello");
    kstring_view sv2("hell");
    test::assert_true(!(sv1 == sv2), "== returns false for views with different lengths");
}

void test_equality_cstring()
{
    kstring_view sv("hello");
    test::assert_true(sv == "hello", "== with implicit const char* conversion returns true for equal content");
    test::assert_true(!(sv == "world"), "== with implicit const char* conversion returns false for different content");
}

void test_inequality()
{
    kstring_view sv1("hello");
    kstring_view sv2("world");
    kstring_view sv3("hello");
    test::assert_true(sv1 != sv2, "!= returns true for views with different content");
    test::assert_true(!(sv1 != sv3), "!= returns false for views with same content");
}

void run()
{
    log::info("Running kstring_view tests...");

    test_cstring_constructor();
    test_cstring_len_constructor();
    test_copy_constructor();
    test_move_constructor();
    test_empty_false();
    test_empty_true();
    test_size_equals_length();
    test_front();
    test_back();
    test_operator_bracket();
    test_c_str();
    test_data();
    test_find_present_at_start();
    test_find_present_in_middle();
    test_find_absent();
    test_find_null();
    test_find_empty_string();
    test_find_longer_than_source();
    test_npos();
    test_substr_from_start();
    test_substr_from_middle();
    test_substr_to_end();
    test_substr_pos_out_of_bounds();
    test_substr_single_char();
    test_copy_assignment();
    test_move_assignment();
    test_equality_equal();
    test_equality_different_content();
    test_equality_different_length();
    test_equality_cstring();
    test_inequality();
}
}

#endif // KERNEL_TESTS
