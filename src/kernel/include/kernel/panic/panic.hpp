#pragma once

namespace kernel {
    [[noreturn]]
    void panic(const char* message);

    [[noreturn]]
    void panicf(const char* format, ...);
}
