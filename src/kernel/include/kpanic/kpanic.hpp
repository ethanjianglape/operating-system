#pragma once

#include <containers/kstring_view.hpp>
#include <kprint/kprint.hpp>

#include <utility>

[[noreturn]]
void kpanic_halt();

template <typename T, typename... Rest>
[[noreturn]]
void kpanicf(kstring_view format, T&& first, Rest&&... rest)
{
    kprintln("*** !! KERNEL PANIC !! ***");
    kprintf(format, first, std::forward<Rest>(rest)..., '\n');
    kprintln("System halted.");

    kpanic_halt();
}

template <typename... Ts>
[[noreturn]]
void kpanic(Ts... args)
{
    kprintln("*** !! KERNEL PANIC !! ***");
    kprintln(args...);
    kprintln("System halted.");

    kpanic_halt();
}
