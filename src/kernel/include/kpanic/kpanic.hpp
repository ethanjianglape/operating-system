#pragma once

#include <containers/kstring_view.hpp>
#include <log/log.hpp>

#include <utility>

[[noreturn]]
void kpanic_halt();

template <typename T, typename... Rest>
[[noreturn]]
void kpanicf(kstring_view format, T&& first, Rest&&... rest)
{
    log::error("*** KERNEL PANIC ***");
    log::errorf(format, first, std::forward<Rest>(rest)...);
    log::error("System halted.");

    kpanic_halt();
}

template <typename... Ts>
[[noreturn]]
void kpanic(Ts... args)
{
    log::error("*** KERNEL PANIC ***");
    log::error(args...);
    log::error("System halted.");

    kpanic_halt();
}
