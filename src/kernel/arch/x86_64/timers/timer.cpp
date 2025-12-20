#include "timer.hpp"

#include <kernel/log/log.hpp>
#include <arch/x86_64/drivers/apic/apic.hpp>

namespace x86_64::timers {
    void timer_tick(std::uint32_t vector) {
        // Nothing for now

        drivers::apic::send_eoi();
    }
}
