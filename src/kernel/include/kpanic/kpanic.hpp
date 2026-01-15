#pragma once

#include <log/log.hpp>

[[noreturn]]
void kpanic_halt();

template <typename... Ts>
[[noreturn]]
void kpanic(Ts... args) {
    log::error("*** KERNEL PANIC ***");
    log::error(args...);
    log::error("System halted.");

    kpanic_halt();
}
