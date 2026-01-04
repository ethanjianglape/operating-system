#include "log/log.hpp"
#include <cstdint>
#include <timer/timer.hpp>

namespace timer {
    static TickHandler handler = nullptr;

    static std::uintmax_t ticks = 0;

    std::uintmax_t get_ticks() { return ticks; }
    
    void tick() {
        ticks++;

        if (handler) {
            handler(ticks);
        }
    }

    void register_handler(TickHandler h) {
        handler = h;
    }
}
