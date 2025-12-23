#pragma once

#include <concepts>
#include <cstdint>

namespace string {
    enum class NumberFormat : std::uint32_t {
        DEC = 1,
        HEX = 2,
        BIN = 3,
        OCT = 4
    };

    constexpr char DEC_CHARS[] = "0123456789";
    constexpr char HEX_CHARS[] = "0123456789ABCDEF";
    constexpr char OCT_CHARS[] = "01234567";
    constexpr char BIN_CHARS[] = "01";
    
    static char buffer[64] = {'\0'};

    inline int number_format_divisor(NumberFormat format) {
        switch (format) {
        case NumberFormat::DEC:
            return 10;
        case NumberFormat::HEX:
            return 16;
        case NumberFormat::OCT:
            return 8;
        case NumberFormat::BIN:
            return 2;
        default:
            return 10;
        }
    }

    inline char number_format_char(int i, NumberFormat format) {
        switch (format) {
        case NumberFormat::DEC:
            return DEC_CHARS[i];
        case NumberFormat::HEX:
            return HEX_CHARS[i];
        case NumberFormat::OCT:
            return OCT_CHARS[i];
        case NumberFormat::BIN:
            return BIN_CHARS[i];
        default:
            return ' ';
        }
    }
    
    inline char* to_string(char c) {
        buffer[0] = c;
        buffer[1] = '\0';
        return buffer;
    }

    inline char* to_string(char* str) {
        return str;
    }

    inline const char* to_string(const char* str) {
        return str;
    }

    inline char* to_string(std::intmax_t num, NumberFormat format = NumberFormat::DEC) {
        if (num == 0) {
            buffer[0] = '0';
            buffer[1] = '\0';
            return buffer;
        }
        
        std::uintmax_t unum;
        const auto divisor = number_format_divisor(format);
        int ri = 0;
        int i = 0;
        char reverse[64];

        if (num < 0) {
            buffer[i++] = '-';
            unum = -static_cast<std::uintmax_t>(num);
        } else {
            unum = num;
        }

        while (unum > 0) {
            const auto index = unum % divisor;
            const auto c = number_format_char(index, format);

            reverse[ri++] = c;
            unum /= divisor;
        }

        while (ri > 0) {
            buffer[i++] = reverse[--ri];
            
        }

        buffer[i] = '\0';

        return buffer;
    }

    inline char* to_string(std::uintmax_t unum, NumberFormat format = NumberFormat::DEC) {
        if (unum == 0) {
            buffer[0] = '0';
            buffer[1] = '\0';
            return buffer;
        }
        
        const auto divisor = number_format_divisor(format);
        int ri = 0;
        int i = 0;
        char reverse[64];

        while (unum > 0) {
            const auto index = unum % divisor;
            const auto c = number_format_char(index, format);

            reverse[ri++] = c;
            unum /= divisor;
        }

        while (ri > 0) {
            buffer[i++] = reverse[--ri];
        }

        buffer[i] = '\0';

        return buffer;
    }

    inline char* to_string(std::signed_integral auto num, NumberFormat format = NumberFormat::DEC) {
        return to_string(static_cast<std::intmax_t>(num), format);
    }

    inline char* to_string(std::unsigned_integral auto unum, NumberFormat format = NumberFormat::DEC) {
        return to_string(static_cast<std::uintmax_t>(unum), format);
    }

    template <typename T>
    concept IsPointer = std::is_pointer_v<T>;

    inline char* to_string(IsPointer auto ptr, NumberFormat format = NumberFormat::DEC) {
        return to_string(reinterpret_cast<std::uintptr_t>(ptr), format);
    }
}
