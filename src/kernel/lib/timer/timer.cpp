#include <cstdint>
#include <kernel/timer/timer.hpp>

namespace kernel::timer {
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
