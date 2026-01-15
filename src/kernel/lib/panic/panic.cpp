#include <panic/panic.hpp>

#include <arch.hpp>
#include <log/log.hpp>

void panic(const char* str) {
    arch::cpu::cli();

    log::error("*** KERNEL PANIC ***");
    log::error(str);
    log::error("System will now halt.");

    arch::cpu::cli();

    while (true) {
        arch::cpu::hlt();
    }
}

void panicf(const char* format, ...) {
    arch::cpu::cli();

    while (true) {
        arch::cpu::hlt();
    }
}
