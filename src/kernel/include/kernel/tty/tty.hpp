#pragma once

#include <containers/kstring.hpp>
#include <cstddef>

namespace kernel::tty {
    void init();
    void reset();

    const kernel::kstring& read_line(std::size_t prompt_start);
}
