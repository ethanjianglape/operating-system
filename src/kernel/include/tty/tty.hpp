#pragma once

#include <containers/kstring.hpp>
#include <cstddef>

namespace tty {
    void init();
    void reset();

    const kstring& read_line(std::size_t prompt_start);
}
