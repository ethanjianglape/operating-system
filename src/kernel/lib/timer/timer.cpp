#include <timer/timer.hpp>
#include <containers/kvector.hpp>
#include <cstdint>

namespace timer {
    static kvector<TickHandler> handlers;
    static std::uintmax_t ticks = 0;

    std::uintmax_t get_ticks() { return ticks; }
   
    void tick(arch::irq::InterruptFrame* frame) {
        ticks++;

        for (const auto& handler : handlers) {
            if (handler) {
                handler(ticks, frame);
            }
        }
    }

    void register_handler(TickHandler h) {
        handlers.push_back(h);
    }
}
