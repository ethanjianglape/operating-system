#pragma once

#include "arch/x86_64/interrupts/irq.hpp"
#include <cstdint>
#include <arch.hpp>

namespace timer {
    using TickHandler = void (*)(std::uintmax_t ticks, arch::irq::InterruptFrame* frame);
    
    void tick(arch::irq::InterruptFrame* frame);
    void register_handler(TickHandler handler);

    std::uintmax_t get_ticks();
}
