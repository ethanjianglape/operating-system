#pragma once

#include <kernel/kprintf/kprintf.hpp>

#include <utility>

namespace kernel::log {
    template <typename... Ts>
    void log_with_prefix(const char* prefix,
                         const char* format,
                         Ts... args) {
        kprintf(prefix);
        kprintf(format, std::forward<Ts>(args)...);
    }
    
    template <typename... Ts>
    void info(const char* format, Ts... args) {
        log_with_prefix("[INFO] ", format, std::forward<Ts>(args)...);
        kprintf("\n");
    }

    template <typename... Ts>
    void init_start(const char* format, Ts... args) {
        log_with_prefix("[INIT] ",format, std::forward<Ts>(args)...);
        kprintf(" initializing...\n");
    }

    template <typename... Ts>
    void init_end(const char* format, Ts... args) {
        log_with_prefix("[INIT] ", format, std::forward<Ts>(args)...);
        kprintf(" initialized!\n");
    }

    template <typename... Ts>
    void warn(const char* format, Ts... args) {
        log_with_prefix("[WARN] ", format, std::forward<Ts>(args)...);
        kprintf("\n");
    }

    template <typename... Ts>
    void error(const char* format, Ts... args) {
        log_with_prefix("[ERROR] ", format, std::forward<Ts>(args)...);
        kprintf("\n");
    }

    template <typename... Ts>
    void success(const char* format, Ts... args) {
        log_with_prefix("[SUCC] ", format, std::forward<Ts>(args)...);
        kprintf("\n");
    }

    template <typename... Ts>
    void debug(const char* format, Ts... args) {
        log_with_prefix("[DEBUG] ", format, std::forward<Ts>(args)...);
        kprintf("\n");
    }
}
