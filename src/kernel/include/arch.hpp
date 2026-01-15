#pragma once

#include <arch/x86_64/memory/vmm.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/drivers/serial/serial.hpp>
#include <arch/x86_64/drivers/apic/apic.hpp>
#include <arch/x86_64/drivers/keyboard/keyboard.hpp>
#include <arch/x86_64/context/context.hpp>
#include <arch/x86_64/entry/entry.hpp>


// Allows kernel library code to indirectly access the current CPU architecture
namespace arch {
    namespace vmm = ::x86_64::vmm;
    namespace cpu = ::x86_64::cpu;
    namespace irq = ::x86_64::irq;
    namespace drivers = ::x86_64::drivers;
    namespace context = ::x86_64::context;
    namespace entry = ::x86_64::entry;
}
