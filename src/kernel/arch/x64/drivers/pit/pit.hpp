#pragma once

#include <cstdint>

/**
 * Legacy 8254 PIT (Programmable Interval Timer)
 *
 * Only used to calibrate the APIC timer, then never touched again.
 * See pit.cpp for detailed documentation.
 */
namespace x64::drivers::pit {

void sleep_ms(std::uint32_t ms);

}
