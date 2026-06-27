#pragma once

#include <kpanic/kpanic.hpp>
#include <source_location>
#include <type_traits>

template <typename T>
concept pointer_type = std::is_pointer_v<T>;

template <pointer_type T>
inline void kassert_not_null(T ptr, const std::source_location loc = std::source_location::current())
{
    if (ptr == nullptr) {
        kpanicf("null pointer at function: {}, file: {}, line: {}", loc.function_name(), loc.file_name(), loc.line());
    }
}

inline void kassert(bool cond, const std::source_location loc = std::source_location::current())
{
    if (!cond) {
        kpanicf("kassert failed at function: {}, file: {}, line: {}", loc.function_name(), loc.file_name(), loc.line());
    }
}

inline void kassert(bool cond, const char* message, const std::source_location loc = std::source_location::current())
{
    if (!cond) {
        kpanicf("{} at function: {}, file: {}, line: {}", message, loc.function_name(), loc.file_name(), loc.line());
    }
}
