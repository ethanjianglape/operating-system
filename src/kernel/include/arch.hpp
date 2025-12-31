#pragma once

#include <arch/x86_64/memory/vmm.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/drivers/serial/serial.hpp>
#include <arch/x86_64/drivers/apic/apic.hpp>
#include <arch/x86_64/drivers/keyboard/keyboard.hpp>
#include <arch/x86_64/process/process.hpp>

// Allows kernel library code to indirectly access the current CPU architecture
namespace arch {
    namespace vmm = ::x86_64::vmm;
    namespace cpu = ::x86_64::cpu;
    namespace drivers = ::x86_64::drivers;
    namespace process = ::x86_64::process;
}
