#pragma once

#include <concepts>

template <std::integral T>
class katomic final {
private:
    T _value;

public:
    katomic()
        : _value{}
    {
    }

    explicit katomic(T value)
        : _value{value}
    {
    }

    katomic(const katomic&) = delete;
    katomic& operator=(const katomic&) = delete;

    T load() const { return __atomic_load_n(&_value, __ATOMIC_SEQ_CST); }
    T exchange(T val) { return __atomic_exchange_n(&_value, val, __ATOMIC_SEQ_CST); }

    void store(T value) { __atomic_store_n(&_value, value, __ATOMIC_SEQ_CST); }

    T operator++() { return __atomic_add_fetch(&_value, 1, __ATOMIC_SEQ_CST); }
    T operator--() { return __atomic_sub_fetch(&_value, 1, __ATOMIC_SEQ_CST); }

    T operator++(int) { return __atomic_fetch_add(&_value, 1, __ATOMIC_SEQ_CST); }
    T operator--(int) { return __atomic_fetch_sub(&_value, 1, __ATOMIC_SEQ_CST); }

    T operator+=(T val) { return __atomic_add_fetch(&_value, val, __ATOMIC_SEQ_CST); }
    T operator-=(T val) { return __atomic_sub_fetch(&_value, val, __ATOMIC_SEQ_CST); }

    operator T() const { return load(); }
};
