#pragma once

#include <memory/memory.hpp>

#include <concepts>
#include <utility>

template <typename T>
concept kunique_ptr_storable = requires {
    requires sizeof(T) <= 4096;
    requires std::destructible<T>;
};

template <kunique_ptr_storable T>
class kunique_ptr final {
private:
    T* _data;

    void destroy()
    {
        if (_data != nullptr) {
            delete _data;
            _data = nullptr;
        }
    }

    template <kunique_ptr_storable U, typename... Args>
    friend kunique_ptr<U> make_kunique(Args... args);

public:
    kunique_ptr()
        : _data{nullptr}
    {
    }

    template <typename... Args>
    explicit kunique_ptr(Args&&... args)
        : _data{new T(std::forward<Args>(args)...)}
    {
    }

    kunique_ptr(const kunique_ptr&) = delete;
    kunique_ptr& operator=(const kunique_ptr&) = delete;

    kunique_ptr(kunique_ptr&& other) noexcept
        : _data{other._data}
    {
        other._data = nullptr;
    }

    kunique_ptr& operator=(kunique_ptr&& other) noexcept
    {
        if (this != &other) {
            destroy();
            _data = other._data;
            other._data = nullptr;
        }

        return *this;
    }

    ~kunique_ptr()
    {
        destroy();
    }

    void reset() { destroy(); }

    T* get() { return _data; }
    const T* get() const { return _data; }

    T* operator->() { return _data; }
    const T* operator->() const { return _data; }

    T& operator*() { return *_data; }
    const T& operator*() const { return *_data; }

    explicit operator bool() const { return _data != nullptr; }
};

template <kunique_ptr_storable T, typename... Args>
kunique_ptr<T> make_kunique(Args... args)
{
    return kunique_ptr<T>(new T(std::forward<Args>(args)...));
}
