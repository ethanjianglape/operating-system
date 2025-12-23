#pragma once

#include <cstddef>

namespace kernel::tty {
    void init();
    void reset();

    char* read_line(std::size_t prompt_start);
}
