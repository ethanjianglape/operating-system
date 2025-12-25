#pragma once

#include <containers/kstring.hpp>
#include <kernel/arch/arch.hpp>
#include <fmt/fmt.hpp>

#include <concepts>

namespace kernel {
    namespace serial = kernel::arch::drivers::serial;

    namespace detail {
        inline void kprint_one(const kstring& str) {
            serial::puts(str.c_str());
        }

        inline void kprint_one(const char* str) {
            serial::puts(str);
        }

        inline void kprint_one(unsigned char* str) {
            serial::puts(str);
        }

        inline void kprint_one(std::integral auto num) {
            serial::puts(fmt::to_string(num));
        }

        template <std::integral T>
        inline void kprint_one(fmt::hex<T> h) {
            serial::puts(fmt::to_string(h));
        }

        template <fmt::ptr_type T>
        inline void kprint_one(fmt::hex<T> h) {
            serial::puts(fmt::to_string(h));
        }

        template <std::integral T>
        inline void kprint_one(fmt::bin<T> b) {
            serial::puts(fmt::to_string(b));
        }

        template <std::integral T>
        inline void kprint_one(fmt::oct<T> o) {
            serial::puts(fmt::to_string(o));
        }

        inline void kprint_one(fmt::ptr_type auto ptr) {
            serial::puts(fmt::to_string(ptr));
        }
    }

    template <typename... Args>
    void kprint(Args... args) {
        (detail::kprint_one(args), ...);
    }

    template <typename... Args>
    void kprintln(Args... args) {
        (detail::kprint_one(args), ..., detail::kprint_one("\n"));
    }
}
