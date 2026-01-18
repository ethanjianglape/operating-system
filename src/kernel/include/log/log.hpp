#pragma once

#include <kprint/kprint.hpp>

namespace log {
    template <typename... Ts>
    void info(Ts... args) {
        kprintln("[INFO] ", args...);
    }

    template <typename... Ts>
    void init_start(Ts... args) {
        kprintln("[INIT] **** ", args..., " initializing ****");
    }

    template <typename... Ts>
    void init_end(Ts... args) {
        kprintln("[INIT] **** ", args..., " initialized! ****");
    }

    template <typename... Ts>
    void warn(Ts... args) {
        kprintln("[WARN] ", args...);
    }

    template <typename... Ts>
    void error(Ts... args) {
        kprintln("[ERROR] ", args...);
    }

    template <typename... Ts>
    void success(Ts... args) {
        kprintln("[SUCCESS] ", args...);
    }

    template <typename... Ts>
    void debug(Ts... args) {
        kprintln("[DEBUG] ", args...);
    }
}
