#include <kernel/kprintf/kprintf.hpp>

#include <cstdarg>
#include <cstddef>

namespace kernel {
    int kprintf(const char* format, ...) {
        std::va_list args;
        va_start(args, format);

        int written = kvprintf(format, args);

        va_end(args);
        
        return written;
    }

    int kvprintf(const char* format, std::va_list args) {
        if (format == nullptr) {
            return 0;
        }

        int written = 0;

        while (*format != '\0') {
            if (*format == '%' && *(format + 1)) {
                format++;

                switch (*format) {
                case 's': {
                    const char* str = va_arg(args, const char*);

                    if (str == nullptr) {
                        str = "(NULL)";
                    }

                    while (*str) {
                        console::putchar(*str++);
                        written++;
                    }

                    break;
                }
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    unsigned int unum;
                    int i = 0;
                    char buff[16];

                    if (num < 0) {
                        console::putchar('-');
                        written++;
                        unum = -static_cast<unsigned int>(num);
                    } else {
                        unum = num;
                    }

                    if (unum == 0) {
                        console::putchar('0');
                        written++;
                    } else {
                        while (unum > 0) {
                            buff[i++] = '0' + (unum % 10);
                            unum /= 10;
                        }

                        while (i > 0) {
                            console::putchar(buff[--i]);
                            written++;
                        }
                    }

                    break;
                }
                case 'u': {
                    unsigned int unum = va_arg(args, unsigned int);
                    int i = 0;
                    char buff[16];

                    if (unum == 0) {
                        console::putchar('0');
                        written++;
                    } else {
                        while (unum > 0) {
                            buff[i++] = '0' + (unum % 10);
                            unum /= 10;
                        }

                        while (i > 0) {
                            console::putchar(buff[--i]);
                            written++;
                        }
                    }

                    break;
                }
                case 'x': {
                    static const char* digits = "0123456789ABCDEF";
                    unsigned int unum = va_arg(args, unsigned int);
                    int i = 0;
                    char buff[16];

                    console::puts("0x", 2);
                    written += 2;

                    if (unum == 0) {
                        console::putchar('0');
                        written++;
                    } else {
                        while (unum > 0) {
                            char digit = digits[unum % 16];
                            buff[i++] = digit;
                            unum /= 16;
                        }

                        while (i > 0) {
                            console::putchar(buff[--i]);
                            written++;
                        }
                    }

                    break;
                }
                case '%':
                    console::putchar('%');
                    written++;
                    break;
                default:
                    console::putchar('%');
                    console::putchar(*format);
                    written += 2;
                    break;
                }
            } else {
                console::putchar(*format);
                written++;
            }

            format++;
        }
        
        return written;
    }
}

namespace kernel::console {
    static driver_config null_driver = {
        .putchar_fn = nullptr,
        .get_color_fn = nullptr,
        .set_color_fn = nullptr,
        .clear_fn = nullptr
    };
    
    static driver_config* driver = &null_driver;

    void init(driver_config* config) {
        driver = config;

        set_color(color::WHITE, color::BLACK);
    }

    void putchar(char c) {
        if (driver->putchar_fn != nullptr) {
            driver->putchar_fn(c);
        }
    }

    void puts(const char* str, std::size_t length) {
        for (std::size_t i = 0; i < length; i++) {
            putchar(str[i]);
        }
    }

    void set_color(color fg, color bg) {
        if (driver->set_color_fn != nullptr) {
            driver->set_color_fn(static_cast<std::uint8_t>(fg), static_cast<std::uint8_t>(bg));
        }
    }

    std::uint8_t get_color() {
        if (driver->get_color_fn != nullptr) {
            return driver->get_color_fn();
        }

        return 0x0F; // Default: white on black
    }

    void clear() {
        if (driver->clear_fn != nullptr) {
            driver->clear_fn();
        }
    }
}
