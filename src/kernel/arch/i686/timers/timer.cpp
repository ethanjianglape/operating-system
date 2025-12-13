#include "timer.hpp"

#include "arch/i686/drivers/apic/apic.hpp"

using namespace i686;

void timer::timer_tick(std::uint32_t vector) {
    // Nothing for now

    drivers::apic::send_eoi();
}
