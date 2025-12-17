#include <kernel/kprintf/kprintf.hpp>
#include <kernel/console/console.hpp>

#include <cstdarg>

namespace kernel {
    int kprintf2(const char* str) {
        int written = 0;

        while (*str) {
            written += kernel::console::put(*str++);
        }

        return written;
    }
    
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

                    console::put("0x");
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
