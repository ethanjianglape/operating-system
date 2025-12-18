#include "timer.hpp"

#include "arch/x86_64/drivers/apic/apic.hpp"

using namespace x86_64;

void timer::timer_tick(std::uint32_t vector) {
    // Nothing for now

    drivers::apic::send_eoi();
}
