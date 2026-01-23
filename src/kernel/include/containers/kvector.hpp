#pragma once

#include <memory/memory.hpp>
#include <crt/crt.h>

#include <cstddef>
#include <concepts>
#include <type_traits>
#include <utility>
#include <initializer_list>
#include <new> // IWYU pragma: keep (required for placement new)

template <typename T>
concept kvector_storable = requires {
    requires sizeof(T) > 0;
    requires sizeof(T) <= 4096;
    requires std::destructible<T>;
    requires std::move_constructible<T> || std::copy_constructible<T>;
};

template <kvector_storable T>
class kvector final {
private:
    static constexpr std::size_t INITIAL_CAPACITY = 16;
    static constexpr std::size_t GROWTH_RATE = 2;

    T* _data;

    std::size_t _size;
    std::size_t _capacity;

    void ensure_capacity(std::size_t new_size) {
        while (_capacity <= new_size) {
            grow();
        }
    }

    void grow() {
        std::size_t new_capacity = _capacity == 0 ? INITIAL_CAPACITY : _capacity * GROWTH_RATE;

        T* new_data = kalloc<T>(new_capacity);

        if constexpr (std::is_trivially_copyable_v<T>) {
            memcpy(new_data, _data, _size * sizeof(T));
        } else {
            for (std::size_t i = 0; i < _size; i++) {
                new (&new_data[i]) T(std::move(_data[i]));

                if constexpr (!std::is_trivially_destructible_v<T>) {
                    _data[i].~T();                    
                }
            }
        }

        kfree(_data);
        _data = new_data;
        _capacity = new_capacity;
    }

  public:
    class iterator {
      private:
        T* _ptr;

      public:
        iterator(T* ptr) : _ptr{ptr} {}
        T& operator*() const { return *_ptr; }
        T* operator->() const { return _ptr; }
        iterator& operator++() {
            _ptr++;
            return *this;
        }
        iterator& operator--() {
            _ptr--;
            return *this;
        }
        iterator& operator+=(std::size_t n) {
            _ptr += n;
            return *this;
        }
        iterator& operator-=(std::size_t n) {
            _ptr -= n;
            return *this;
        }
        bool operator==(const iterator& other) const { return _ptr == other._ptr; }
        bool operator!=(const iterator& other) const { return _ptr != other._ptr; }
    };

    class const_iterator {
      private:
        const T* _ptr;

      public:
        const_iterator(const T* ptr) : _ptr{ptr} {}
        const T& operator*() const { return *_ptr; }
        const T* operator->() const { return _ptr; }
        const_iterator& operator++() {
            _ptr++;
            return *this;
        }
        const_iterator& operator--() {
            _ptr--;
            return *this;
        }
        const_iterator& operator+=(std::size_t n) {
            _ptr += n;
            return *this;
        }
        const_iterator& operator-=(std::size_t n) {
            _ptr -= n;
            return *this;
        }
        bool operator==(const const_iterator& other) const { return _ptr == other._ptr; }
        bool operator!=(const const_iterator& other) const { return _ptr != other._ptr; }
    };

    kvector() {
        _data = nullptr;
        _size = 0;
        _capacity = 0;
    }

    kvector(std::initializer_list<T> init) : kvector{} {
        ensure_capacity(init.size());

        if constexpr (std::is_trivially_copyable_v<T>) {
            for (const auto& val : init) {
                _data[_size++] = val;
            }
        } else {
            for (const auto& val : init) {
                new (&_data[_size++]) T(val);
            }
        }
    }

    explicit kvector(std::size_t count, const T& value = {}) : kvector{} {
        ensure_capacity(count);

        for (std::size_t i = 0; i < count; i++) {
            push_back(value);
        }
    }

    kvector(kvector&& other) : _data{other._data}, _size{other._size}, _capacity{other._capacity} {
        other._data = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    kvector(const kvector& other) : kvector{} {
        if (other.empty()) {
            return;
        }

        ensure_capacity(other.size());

        if constexpr (std::is_trivially_copyable_v<T>) {
            memcpy(_data, other._data, other.size() * sizeof(T));
        } else {
            for (std::size_t i = 0; i < other.size(); i++) {
                new (&_data[i]) T(other._data[i]);
            }
        }

        _size = other.size();
    }

    kvector& operator=(kvector&& other) {
        if (this != &other) {
            clear();
            kfree(_data);
            _data = other._data;
            _size = other._size;
            _capacity = other._capacity;
            other._data = nullptr;
            other._size = 0;
            other._capacity = 0;
        }

        return *this;
    }

    kvector& operator=(const kvector& other) {
        if (this != &other) {
            clear();
            kfree(_data);
            _data = nullptr;
            _capacity = 0;

            ensure_capacity(other.size());

            if constexpr (std::is_trivially_copyable_v<T>) {
                memcpy(_data, other._data, other.size() * sizeof(T));
            } else {
                for (std::size_t i = 0; i < other.size(); i++) {
                    new (&_data[i]) T(other._data[i]);
                }
            }

            _size = other.size();
        }

        return *this;
    }

    kvector& operator=(std::initializer_list<T> init) {
        clear();
        ensure_capacity(init.size());
        for (const auto& val : init) {
            push_back(val);
        }
        return *this;
    }

    ~kvector() {
        clear();
        kfree(_data);
    }

    void reserve(std::size_t capacity) { ensure_capacity(capacity); }

    std::size_t size() const { return _size; }

    bool empty() const { return _size == 0; }

    iterator begin() { return iterator{_data}; }
    iterator end() { return iterator{_data + _size}; }

    const_iterator begin() const { return const_iterator{_data}; }
    const_iterator end() const { return const_iterator{_data + _size}; }

    T* data() { return _data; }
    const T* data() const { return _data; }

    T& front() { return _data[0]; }
    const T& front() const { return _data[0]; }

    T& back() { return _data[_size - 1]; }
    const T& back() const { return _data[_size - 1]; }

    T& operator[](std::size_t i) { return _data[i]; }
    const T& operator[](std::size_t i) const { return _data[i]; }

    T& push_back(const T& t) {
        ensure_capacity(_size + 1);

        if constexpr (std::is_trivially_copyable_v<T>) {
            _data[_size] = t;
        } else {
            new (&_data[_size]) T(t);
        }

        return _data[_size++];
    }

    T& push_back(T&& t) {
        ensure_capacity(_size + 1);

        if constexpr (std::is_trivially_copyable_v<T>) {
            _data[_size] = t;
        } else {
            new (&_data[_size]) T(std::move(t));
        }

        return _data[_size++];
    }

    template <typename... Args> T& emplace_back(Args&&... args) {
        ensure_capacity(_size + 1);
        new (&_data[_size]) T(std::forward<Args>(args)...);
        return _data[_size++];
    }

    void pop_back() {
        if (empty()) {
            return;
        }

        if constexpr (std::is_trivially_destructible_v<T>) {
            _size--;
        } else {
            _data[--_size].~T();
        }
    }

    void clear() {
        if (empty()) {
            return;
        }

        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (std::size_t i = 0; i < _size; i++) {
                _data[i].~T();
            }
        }

        _size = 0;
    }

    void erase(std::size_t pos) {
        if (pos >= _size) {
            return;
        }

        for (std::size_t i = pos; i < _size; i++) {
            _data[i] = _data[i + 1];
        }

        _size--;
    }

    void move_to_end(std::size_t pos) {
        for (std::size_t i = pos; i < _size - 1; i++) {
            T temp = _data[i];
            _data[i] = _data[i + 1];
            _data[i + 1] = temp;
        }
    }
};
