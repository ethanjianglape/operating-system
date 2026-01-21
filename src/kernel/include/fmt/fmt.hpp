#pragma once

#include <containers/kstring.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace fmt {
    template <typename T> struct hex { T value; };
    template <typename T> struct bin { T value; };
    template <typename T> struct oct { T value; };

    template <typename T> hex(T) -> hex<T>;
    template <typename T> bin(T) -> bin<T>;
    template <typename T> oct(T) -> oct<T>;

    template <typename Ptr>
    concept ptr_type = std::is_pointer_v<Ptr>;

    constexpr char DEC_CHARS[] = "0123456789";
    constexpr char HEX_CHARS[] = "0123456789ABCDEF";
    constexpr char OCT_CHARS[] = "01234567";
    constexpr char BIN_CHARS[] = "01";

    enum class NumberFormat : std::uint32_t {
        DEC = 1,
        HEX = 2,
        BIN = 3,
        OCT = 4
    };

    static char buffer[128] = {'\0'};

    static std::size_t index = 0;

    inline bool is_numeric(char c) {
        return c >= '0' && c <= '9';
    }

    inline bool is_alpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

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

    inline void insert(std::size_t pos, std::size_t num, char pad) {
        if (pos >= index) {
            return;
        }
        
        std::size_t i = index - 1;

        while (true) {
            buffer[i + num] = buffer[i];

            if (i == pos) {
                break;
            }

            i--;
        }

        while (i < pos + num) {
            buffer[i++] = pad;
        }

        index += num;
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

    inline const char* to_string(std::uintmax_t unum, NumberFormat format = NumberFormat::DEC) {
        index = 0;
        
        if (unum == 0) {
            buffer[index++] = '0';
            buffer[index++] = '\0';
            return buffer;
        }

        if (format == NumberFormat::HEX) {
            buffer[index++] = '0';
            buffer[index++] = 'x';
        } else if (format == NumberFormat::BIN) {
            buffer[index++] = '0';
            buffer[index++] = 'b';
        } else if (format == NumberFormat::OCT) {
            buffer[index++] = '0';
        }
        
        const auto divisor = number_format_divisor(format);
        int ri = 0;
        char reverse[64];

        while (unum > 0) {
            const auto index = unum % divisor;
            const auto c = number_format_char(index, format);

            reverse[ri++] = c;
            unum /= divisor;
        }

        while (ri > 0) {
            buffer[index++] = reverse[--ri];
        }

        // Pad hex and bin formats with 0 so they fit within 8/16/32/64 bits
        // ex: 0x1234 -> 0x00001234, 0b1101 -> 0b0000101
        if (format == NumberFormat::HEX || format == NumberFormat::BIN) {
            std::size_t len = index - 2;
            std::size_t zeros = (8 - len) & 7;

            insert(2, zeros, '0');
        }

        buffer[index] = '\0';

        return buffer;
    }

    inline const char* to_string(std::intmax_t num, NumberFormat format = NumberFormat::DEC) {
        index = 0;
        std::uintmax_t unum;

        if (num < 0) {
            buffer[index++] = '-';
            unum = -static_cast<std::uintmax_t>(num);
        } else {
            unum = num;
        }

        if (unum == 0) {
            buffer[index++] = '0';
            buffer[index++] = '\0';
            return buffer;
        }

        if (format == NumberFormat::HEX) {
            buffer[index++] = '0';
            buffer[index++] = 'x';
        } else if (format == NumberFormat::BIN) {
            buffer[index++] = '0';
            buffer[index++] = 'b';
        } else if (format == NumberFormat::OCT) {
            buffer[index++] = '0';
        }
        
        const auto divisor = number_format_divisor(format);
        int ri = 0;
        char reverse[64];

        while (unum > 0) {
            const auto index = unum % divisor;
            const auto c = number_format_char(index, format);

            reverse[ri++] = c;
            unum /= divisor;
        }

        while (ri > 0) {
            buffer[index++] = reverse[--ri];
        }

        // Pad hex and bin formats with 0 so they fit within 8/16/32/64 bits
        // ex: 0x1234 -> 0x00001234, 0b1101 -> 0b0000101
        if (format == NumberFormat::HEX || format == NumberFormat::BIN) {
            std::size_t len = index - 2;
            std::size_t zeros = (8 - len) & 7;

            insert(2, zeros, '0');
        }

        buffer[index] = '\0';

        return buffer;
    }

    inline const char* to_string(std::signed_integral auto num, NumberFormat format = NumberFormat::DEC) {
        return to_string(static_cast<std::intmax_t>(num), format);
    }

    inline const char* to_string(std::unsigned_integral auto num, NumberFormat format = NumberFormat::DEC) {
        return to_string(static_cast<std::uintmax_t>(num), format);
    }

    template <std::integral T>
    inline const char* to_string(hex<T> h) {
        return to_string(h.value, NumberFormat::HEX);
    }

    template <ptr_type T>
    inline const char* to_string(hex<T> h) {
        return to_string(reinterpret_cast<std::uintptr_t>(h.value), NumberFormat::HEX);
    }

    template <std::integral T>
    inline const char* to_string(bin<T> h) {
        return to_string(h.value, NumberFormat::BIN);
    }

    template <std::integral T>
    inline const char* to_string(oct<T> h) {
        return to_string(h.value, NumberFormat::OCT);
    }

    inline const char* to_string(ptr_type auto ptr) {
        return to_string(reinterpret_cast<std::uintptr_t>(ptr), NumberFormat::HEX);
    }

    inline const char* to_string(const kstring& str) {
        return str.c_str();
    }

    inline std::uintmax_t parse_uint(const char* str, std::size_t len, NumberFormat format = NumberFormat::DEC) {
        const auto divisor = number_format_divisor(format);
        
        std::uintmax_t result = 0;

        for (std::size_t i = 0; i < len && str[i]; i++) {
            result = result * divisor + (str[i] - '0');
        }

        return result;
    }

    inline std::uintmax_t parse_uint(const kstring& str, NumberFormat format = NumberFormat::DEC) {
        return parse_uint(str.c_str(), str.size(), format);
    }

    inline int parse_int(char c) {
        return c - '0';
    }
}
