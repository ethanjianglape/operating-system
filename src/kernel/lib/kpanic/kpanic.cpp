#include <kpanic/kpanic.hpp>
#include <arch.hpp>

[[noreturn]]
void kpanic_halt() {
    arch::cpu::cli();

    while (true) {
        arch::cpu::hlt();
    }
}
