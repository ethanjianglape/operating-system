#include "timer.hpp"

#include "arch/i386/drivers/apic/apic.hpp"

using namespace i386;

void timer::timer_tick(std::uint32_t vector) {
    // Nothing for now

    drivers::apic::send_eoi();
}
