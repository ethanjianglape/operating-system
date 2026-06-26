#include <arch.hpp>
#include <kpanic/kpanic.hpp>

[[noreturn]]
void kpanic_halt()
{
    arch::percpu::disable_preemption();
    arch::cpu::cli();

    while (true) {
        arch::cpu::hlt();
    }

    __builtin_unreachable();
}
