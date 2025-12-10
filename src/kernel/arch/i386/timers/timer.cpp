#include "timer.hpp"
#include "arch/i386/apic/apic.hpp"

using namespace i386;

void timer::timer_tick(std::uint32_t vector) {
    // Nothing for now

    apic::send_eoi();
}
