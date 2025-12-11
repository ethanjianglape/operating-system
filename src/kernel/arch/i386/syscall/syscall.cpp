#include "syscall.hpp"
#include "arch/i386/interrupts/irq.hpp"

using namespace i386;

extern "C"
void syscall_dispatcher(syscall::registers* registers) {
    const auto syscall_num = registers->eax;

    registers->eax = 0;
}

void syscall::init() {
}
