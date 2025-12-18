#pragma once

#include <kernel/console/console.hpp>

#include <utility>

namespace kernel {
    inline int kprintf(const char* str) {
        int written = 0;

        while (*str) {
            written += kernel::console::put(*str++);
        }

        return written;
    }

    template <typename T, typename... Ts>
    int kprintf(const char* format, T value, Ts... args) {
        if (format == nullptr) {
            return 0;
        }

        int written = 0;

        for (; *format; format++) {
            if (*format == '%' && (*format + 1)) {
                switch (*(format + 1)) {
                case '%':
                    written += console::put('%');
                    break;
                case 'b':
                    console::set_number_format(console::NumberFormat::BIN);
                    written += console::put("0b");
                    written += console::put(value);
                    console::reset_number_format();
                    break;
                case 'o':
                    console::set_number_format(console::NumberFormat::OCT);
                    written += console::put('0');
                    written += console::put(value);
                    console::reset_number_format();
                    break;
                case 'i':
                case 'd':
                    console::set_number_format(console::NumberFormat::DEC);
                    written += console::put(value);
                    console::reset_number_format();
                    break;
                case 'x':
                    console::set_number_format(console::NumberFormat::HEX);
                    written += console::put("0x");
                    written += console::put(value);
                    console::reset_number_format();
                    break;
                default:
                    written += console::put(value);
                }
                
                written += kprintf(format + 2, std::forward<Ts>(args)...);

                return written;
            } 

            written += console::put(*format);
        }

        return written;
    }
}
