#pragma once

#include <arch/x64/context/context.hpp>
#include <arch/x64/cpu/cpu.hpp>
#include <arch/x64/drivers/apic/apic.hpp>
#include <arch/x64/drivers/keyboard/keyboard.hpp>
#include <arch/x64/drivers/serial/serial.hpp>
#include <arch/x64/drivers/tsc/tsc.hpp>
#include <arch/x64/gdt/gdt.hpp>
#include <arch/x64/memory/vmm.hpp>
#include <arch/x64/percpu/percpu.hpp>
#include <arch/x64/tls/tls.hpp>
#include <arch/x64/trap/syscall_entry.hpp>

// Allows kernel library code to indirectly access the current CPU architecture
namespace arch {
namespace vmm = ::x64::vmm;
namespace cpu = ::x64::cpu;
namespace irq = ::x64::irq;
namespace drivers = ::x64::drivers;
namespace context = ::x64::context;
namespace trap = ::x64::trap;
namespace percpu = ::x64::percpu;
namespace tls = ::x64::tls;
namespace gdt = ::x64::gdt;
}
