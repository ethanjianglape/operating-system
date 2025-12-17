#include <kernel/log/log.hpp>
#include <kernel/kprintf/kprintf.hpp>
#include <kernel/console/console.hpp>

#include <cstdarg>
#include <cstdint>


namespace kernel::log {
    namespace detail {
        void log_with_color(console::RgbColor fg, console::RgbColor bg, const char* str) {
            const auto saved_fg = console::get_current_fg();
            const auto saved_bg = console::get_current_bg();

            console::set_color(fg, bg);

            kprintf("%s", str);

            console::set_color(static_cast<console::RgbColor>(saved_fg),
                               static_cast<console::RgbColor>(saved_bg));
        }
        
        void log_with_color(console::RgbColor fg,
                            console::RgbColor bg,
                            const char* prefix,
                            const char* format,
                            std::va_list args) {
            const auto saved_fg = console::get_current_fg();
            const auto saved_bg = console::get_current_bg();

            console::set_color(fg, bg);

            kprintf("%s", prefix);

            console::set_color(static_cast<console::RgbColor>(saved_fg),
                               static_cast<console::RgbColor>(saved_bg));

            kvprintf(format, args);
        }
    }

    void info(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::RgbColor::CYAN, console::RgbColor::BLACK, "[INFO] ", format, args);
        kprintf("\n");
        va_end(args);
    }

    void error(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::RgbColor::RED, console::RgbColor::BLACK, "[ERROR] ", format, args);
        kprintf("\n");
        va_end(args);
    }

    void warn(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::RgbColor::WHITE, console::RgbColor::BLACK, "[WARN] ", format, args);
        kprintf("\n");
        va_end(args);
    }

    void init_start(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::RgbColor::WHITE, console::RgbColor::BLACK, "[INIT] ", format, args);
        kprintf(" initializing...\n");
        va_end(args);
    }

    void init_end(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::RgbColor::GREEN, console::RgbColor::BLACK, "[INIT] ", format, args);
        kprintf(" initialized!\n");
        va_end(args);
    }

    void success(const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        detail::log_with_color(console::RgbColor::GREEN, console::RgbColor::BLACK, "[SUCC] ", format, args);
        kprintf("\n");
        va_end(args);
    }
}
