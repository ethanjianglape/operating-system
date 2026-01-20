#pragma once

#include <containers/kstring.hpp>
#include <arch.hpp>
#include <fmt/fmt.hpp>

#include <concepts>

namespace serial = arch::drivers::serial;

namespace kprint_detail {
    inline void print_one(const kstring& str) {
        serial::puts(str.c_str());
    }

    inline void print_one(const char* str) {
        serial::puts(str);
    }

    inline void print_one(bool b) {
        serial::puts(b ? "true" : "false");
    }

    inline void print_one(unsigned char* str) {
        serial::puts(str);
    }

    inline void print_one(std::integral auto num) {
        serial::puts(fmt::to_string(num));
    }

    template <std::integral T>
    inline void print_one(fmt::hex<T> h) {
        serial::puts(fmt::to_string(h));
    }

    template <fmt::ptr_type T>
    inline void print_one(fmt::hex<T> h) {
        serial::puts(fmt::to_string(h));
    }

    template <std::integral T>
    inline void print_one(fmt::bin<T> b) {
        serial::puts(fmt::to_string(b));
    }

    template <std::integral T>
    inline void print_one(fmt::oct<T> o) {
        serial::puts(fmt::to_string(o));
    }

    inline void print_one(fmt::ptr_type auto ptr) {
        serial::puts(fmt::to_string(ptr));
    }
}

template <typename... Args>
void kprint(Args... args) {
    (kprint_detail::print_one(args), ...);
}

template <typename... Args>
void kprintln(Args... args) {
    (kprint_detail::print_one(args), ..., kprint_detail::print_one("\n"));
}
