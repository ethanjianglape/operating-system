#include <arch.hpp>
#include <kpanic/kpanic.hpp>

[[noreturn]]
void kpanic_halt()
{
    arch::cpu::cli();

    while (true) {
        arch::cpu::hlt();
    }
}
