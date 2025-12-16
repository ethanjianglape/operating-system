#pragma once

#include <cstdarg>

namespace kernel {
    int kprintf(const char* str, ...);

    int kvprintf(const char* str, std::va_list args);
}
