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
    static console_putchar_fn putchar_driver_fn = nullptr;
    static console_set_color_fn set_color_driver_fn = nullptr;
    static console_get_color_fn get_color_driver_fn = nullptr;

    void init(console_putchar_fn putchar_fn,
              console_set_color_fn set_color_fn,
              console_get_color_fn get_color_fn) {
        set_putchar_driver(putchar_fn);
        set_color_driver(set_color_fn, get_color_fn);
        
        set_color(color::WHITE, color::BLACK);
    }
    
    void set_putchar_driver(console_putchar_fn fn) {
        putchar_driver_fn = fn;
    }

    void set_color_driver(console_set_color_fn set_fn, console_get_color_fn get_fn) {
        set_color_driver_fn = set_fn;
        get_color_driver_fn = get_fn;
    }

    void putchar(char c) {
        if (putchar_driver_fn != nullptr) {
            putchar_driver_fn(c);
        }
    }

    void puts(const char* str, std::size_t length) {
        for (std::size_t i = 0; i < length; i++) {
            putchar(str[i]);
        }
    }

    void set_color(color fg, color bg) {
        if (set_color_driver_fn != nullptr) {
            set_color_driver_fn(static_cast<std::uint8_t>(fg), static_cast<std::uint8_t>(bg));
        }
    }

    std::uint8_t get_color() {
        if (get_color_driver_fn != nullptr) {
            return get_color_driver_fn();
        }

        return 0x0F; // Default: white on black
    }
}
