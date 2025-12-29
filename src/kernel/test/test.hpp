#pragma once

#ifdef KERNEL_TESTS

#include <cstddef>
#include <source_location>

namespace test {
    struct Results {
        std::size_t passed;
        std::size_t failed;
    };

    // Track test results
    void pass(const char* name);
    void fail(const char* name, const std::source_location& loc);

    // Run all registered test suites and print summary
    void run_all();

    Results get_results();

    // Assertion functions - continue on failure (no panic)
    inline void assert_true(bool cond, const char* name,
                            std::source_location loc = std::source_location::current()) {
        if (cond) {
            pass(name);
        } else {
            fail(name, loc);
        }
    }

    template <typename T, typename U>
    inline void assert_eq(const T& actual, const U& expected, const char* name,
                          std::source_location loc = std::source_location::current()) {
        assert_true(actual == expected, name, loc);
    }

    template <typename T, typename U>
    inline void assert_ne(const T& actual, const U& expected, const char* name,
                          std::source_location loc = std::source_location::current()) {
        assert_true(actual != expected, name, loc);
    }

    template <typename T>
    inline void assert_not_null(const T* ptr, const char* name,
                                std::source_location loc = std::source_location::current()) {
        assert_true(ptr != nullptr, name, loc);
    }

    template <typename T>
    inline void assert_null(const T* ptr, const char* name,
                            std::source_location loc = std::source_location::current()) {
        assert_true(ptr == nullptr, name, loc);
    }
}

#endif // KERNEL_TESTS
