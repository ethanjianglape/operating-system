#pragma once

#include <kernel/arch/arch.hpp>
#include <string/string.hpp>

#include <utility>

namespace kernel {
    namespace serial = kernel::arch::drivers::serial;
    
    inline int kprintf(const char* str) {
        return serial::puts(str);
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
                    written += serial::putchar('%');
                    break;
                case 'b':
                    written += serial::puts("0b");
                    written += serial::puts(string::to_string(value, string::NumberFormat::BIN));
                    break;
                case 'o':
                    written += serial::puts("0");
                    written += serial::puts(string::to_string(value, string::NumberFormat::OCT));
                    break;
                case 'i':
                case 'd':
                    written += serial::puts(string::to_string(value));
                    break;
                case 'x':
                    written += serial::puts("0x");
                    written += serial::puts(string::to_string(value, string::NumberFormat::HEX));
                    break;
                default:
                    written += serial::puts(string::to_string(value));
                    break;
                }
                
                written += kprintf(format + 2, std::forward<Ts>(args)...);

                return written;
            } 

            written += serial::putchar(*format);
        }

        return written;
    }
}
