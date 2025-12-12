#pragma once

namespace kernel::log {
    void info(const char* format, ...);
    void init_start(const char* format, ...);
    void init_end(const char* format, ...);
    void warn(const char* format, ...);
    void error(const char* format, ...);
    void success(const char* format, ...);
    void debug(const char* format, ...);
}
