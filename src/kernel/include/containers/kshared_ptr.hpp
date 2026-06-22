#pragma once

#include <exclusive/katomic.hpp>
#include <memory/memory.hpp>

#include <concepts>
#include <utility>

template <typename T>
concept kshared_ptr_storable = requires {
    requires sizeof(T) <= 4096;
    requires std::destructible<T>;
};

template <kshared_ptr_storable T>
class kshared_ptr final {
private:
    T* _data;
    katomic<int>* _refs;

    void inc_refs()
    {
        if (_refs != nullptr) {
            ++(*_refs);
        }
    }

    void dec_refs()
    {
        if (_refs != nullptr) {
            --(*_refs);
        }
    }

    void destroy_if_no_refs()
    {
        if (_data != nullptr && _refs != nullptr && _refs->load() == 0) {
            delete _data;
            delete _refs;
            _data = nullptr;
            _refs = nullptr;
        }
    }

    kshared_ptr(T* data)
        : _data{data}
        , _refs{new katomic<int>{1}}
    {
    }

    template <kshared_ptr_storable U, typename... Args>
    friend kshared_ptr<U> make_kshared(Args... args);

public:
    kshared_ptr()
        : _data{nullptr}
        , _refs{nullptr}
    {
    }

    kshared_ptr(const kshared_ptr& other)
        : _data{other._data}
        , _refs{other._refs}
    {
        inc_refs();
    }

    kshared_ptr(kshared_ptr&& other) noexcept
        : _data{other._data}
        , _refs{other._refs}
    {
        other._data = nullptr;
        other._refs = nullptr;
    }

    kshared_ptr& operator=(const kshared_ptr& other)
    {
        if (this != &other) {
            _data = other._data;
            _refs = other._refs;
            inc_refs();
        }

        return *this;
    }

    kshared_ptr& operator=(kshared_ptr&& other)
    {
        if (this != &other) {
            dec_refs();
            destroy_if_no_refs();
            _data = other._data;
            _refs = other._refs;
            other._data = nullptr;
            other._refs = nullptr;
        }

        return *this;
    }

    ~kshared_ptr()
    {
        dec_refs();
        destroy_if_no_refs();
    }

    T* get() { return _data; }
    const T* get() const { return _data; }

    T* operator->() { return _data; }
    const T* operator->() const { return _data; }

    T& operator*() { return *_data; }
    const T& operator*() const { return *_data; }

    explicit operator bool() const { return _data != nullptr; }
};

template <kshared_ptr_storable T, typename... Args>
kshared_ptr<T> make_kshared(Args... args)
{
    return kshared_ptr<T>(new T(std::forward<Args>(args)...));
}
