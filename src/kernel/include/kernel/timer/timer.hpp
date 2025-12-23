#pragma once

#include <cstdint>
namespace kernel::timer {
    using TickHandler = void (*)(std::uintmax_t ticks);
    
    void tick();
    void register_handler(TickHandler handler);

    std::uintmax_t get_ticks();
}
