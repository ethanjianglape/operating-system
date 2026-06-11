#pragma once

#include "algo/algo.hpp"
#include "arch/x86_64/drivers/serial/serial.hpp"
#include "containers/klist.hpp"
#include "containers/kvector.hpp"
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

    template <typename T>
    inline void print_one(const klist<T>& list) {
        serial::putchar('[');
        serial::puts(algo::join(list, ','));
        serial::putchar(']');
    }

    template <typename T>
    inline void print_one(const kvector<T>& v) {
        serial::putchar('[');
        serial::puts(algo::join(v, ','));
        serial::putchar(']');
    }

    inline void print_one(bool b) {
        serial::puts(b ? "true" : "false");
    }

    inline void print_one(unsigned char* str) {
        serial::puts(str);
    }

    inline void print_one(char c) {
        serial::putchar(c);
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

inline void kprintf(const kstring& format) {
    serial::puts(format);
}

template <typename T, typename... Rest>
void kprintf(const kstring& format, T first, Rest... rest) {
    const auto pos = format.find("{}");

    if (pos != kstring::npos) {
        kprint_detail::print_one(format.substr(0, pos));
        kprint_detail::print_one(first);

        kprintf(format.substr(pos + 2), rest...);
    } else {
        kprint_detail::print_one(format);
    }
}

inline void kprintln() {
    kprint_detail::print_one('\n');
}

template <typename... Args>
void kprintln(Args... args) {
    (kprint_detail::print_one(args), ..., kprintln());
}
