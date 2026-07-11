#pragma once

#include <containers/kstring.hpp>
#include <containers/kstring_view.hpp>
#include <kprint/kprint.hpp>

#include <utility>

namespace log {

template <typename... Ts>
void info(Ts... args)
{
    kprintln("[INFO] ", args...);
}

template <typename... Ts>
void init_start(Ts... args)
{
    kprintln("[INITIALIZE] ", args...);
}

template <typename... Ts>
void init_end(Ts... args)
{
    kprintln("[INITIALIZED] ", args...);
}

template <typename... Ts>
void warn(Ts... args)
{
    kprintln("[WARN] ", args...);
}

template <typename... Ts>
void error(Ts... args)
{
    kprintln("[ERROR] ", args...);
}

template <typename T, typename... Rest>
void errorf(kstring_view format, T&& first, Rest&&... rest)
{
    kprint("[ERROR] ");
    kprintf(format, first, std::forward<Rest>(rest)...);
    kprintln();
}

template <typename... Ts>
void success(Ts... args)
{
    kprintln("[SUCCESS] ", args...);
}

template <typename... Ts>
void debug(Ts... args)
{
    kprintln("[DEBUG] ", args...);
}

inline void debugf(kstring_view format)
{
    kprint("[DEBUG] ");
    kprint(format);
    kprintln();
}

template <typename T, typename... Rest>
void debugf(kstring_view format, T first, Rest... rest)
{
    kprint("[DEBUG] ");
    kprintf(format, first, rest...);
    kprintln();
}

}
