#pragma once

#include <kernel/memory/memory.hpp>
#include <crt/crt.h>

#include <cstddef>

namespace kernel {
    class kstring final {
    private:
        static constexpr std::size_t INITIAL_CAPACITY = 4096;
        static constexpr std::size_t GROWTH_RATE = 2;

        char* _data;
        
        std::size_t _length;
        std::size_t _capacity;

        void ensure_capacity(std::size_t new_size) {
            while (_capacity < new_size) {
                grow();
            }
        }

        void grow() {
            std::size_t new_capacity = _capacity == 0 ? INITIAL_CAPACITY : _capacity * GROWTH_RATE;

            char* new_data = kalloc<char>(new_capacity);

            if (_data) {
                memcpy(new_data, _data, _length);
                kfree(_data);
            }
            
            _data = new_data;
            _capacity = new_capacity;
        }

    public:
        class iterator {
        private:
            char* _ptr;
        public:
            iterator(char* ptr) : _ptr{ptr} {}
            char& operator*() const { return *_ptr; }
            char* operator->() const { return _ptr; }
            iterator& operator++() { _ptr++; return *this; }
            iterator& operator--() { _ptr--; return *this; }
            iterator& operator+=(std::size_t n) { _ptr += n; return *this; }
            iterator& operator-=(std::size_t n) { _ptr -= n; return *this; }
            bool operator==(const iterator& other) const { return _ptr == other._ptr; }
            bool operator!=(const iterator& other) const { return _ptr != other._ptr; }
        };

        class const_iterator {
        private:
            const char* _ptr;
        public:
            const_iterator(const char* ptr) : _ptr{ptr} {}
            const char& operator*() const { return *_ptr; }
            const char* operator->() const { return _ptr; }
            const_iterator& operator++() { _ptr++; return *this; }
            const_iterator& operator--() { _ptr--; return *this; }
            const_iterator& operator+=(std::size_t n) { _ptr += n; return *this; }
            const_iterator& operator-=(std::size_t n) { _ptr -= n; return *this; }
            bool operator==(const const_iterator& other) const { return _ptr == other._ptr; }
            bool operator!=(const const_iterator& other) const { return _ptr != other._ptr; }
        };

        // Default constructor
        kstring() : _data{nullptr}, _length{0}, _capacity{0} {}

        // Construct from C string
        kstring(const char* s) : kstring{} {
            if (s == nullptr) {
                return;
            }

            std::size_t len = strlen(s);
            ensure_capacity(len + 1);
            memcpy(_data, s, len + 1);
            _length = len;
        }

        // Move constructor
        kstring(kstring&& other) noexcept
            : _data{other._data}, _length{other._length}, _capacity{other._capacity}
        {
            other._data = nullptr;
            other._length = 0;
            other._capacity = 0;
        }

        // Copy constructor
        kstring(const kstring& other) : kstring{} {
            if (other.empty()) {
                return;
            }

            ensure_capacity(other._length + 1);
            memcpy(_data, other._data, other._length + 1);  // include null terminator
            _length = other._length;
        }

        // Move assignment
        kstring& operator=(kstring&& other) noexcept {
            if (this != &other) {
                kfree(_data);
                _data = other._data;
                _length = other._length;
                _capacity = other._capacity;
                other._data = nullptr;
                other._length = 0;
                other._capacity = 0;
            }
            
            return *this;
        }

        // Copy assignment
        kstring& operator=(const kstring& other) {
            if (this != &other) {
                kfree(_data);
                _data = nullptr;
                _length = 0;
                _capacity = 0;

                if (!other.empty()) {
                    ensure_capacity(other._length + 1);
                    memcpy(_data, other._data, other._length + 1);
                    _length = other._length;
                }
            }
            
            return *this;
        }

        // Assign from C string
        kstring& operator=(const char* s) {
            kfree(_data);
            _data = nullptr;
            _length = 0;
            _capacity = 0;

            if (s != nullptr) {
                std::size_t len = strlen(s);
                ensure_capacity(len + 1);
                memcpy(_data, s, len + 1);
                _length = len;
            }
            return *this;
        }

        // Destructor
        ~kstring() {
            kfree(_data);
        }

        // Capacity
        void reserve(std::size_t capacity) {
            ensure_capacity(capacity);
        }

        std::size_t length() const { return _length; }
        std::size_t size() const { return _length; }
        bool empty() const { return _length == 0; }

        // Iterators
        iterator begin() { return iterator{_data}; }
        iterator end() { return iterator{_data + _length}; }
        const_iterator begin() const { return const_iterator{_data}; }
        const_iterator end() const { return const_iterator{_data + _length}; }

        // Element access
        char& front() { return _data[0]; }
        const char& front() const { return _data[0]; }

        char& back() { return _data[_length - 1]; }
        const char& back() const { return _data[_length - 1]; }

        char& operator[](std::size_t i) { return _data[i]; }
        const char& operator[](std::size_t i) const { return _data[i]; }

        const char* c_str() const { return _data ? _data : ""; }
        char* data() { return _data; }
        const char* data() const { return _data; }

        // Modifiers
        char& push_back(char c) {
            ensure_capacity(_length + 2);  // +1 for char, +1 for null terminator
            _data[_length] = c;
            _data[_length + 1] = '\0';
            return _data[_length++];
        }

        void pop_back() {
            if (empty()) {
                return;
            }
            _data[--_length] = '\0';
        }

        void clear() {
            if (_data) {
                _data[0] = '\0';
            }
            _length = 0;
        }

        void reverse() {
            if (empty()) {
                return;
            }

            char* s = _data;
            char* e = _data + _length - 1;

            while (s < e) {
                char t = *s;
                *s = *e;
                *e = t;

                s++;
                e--;
            }
        }

        // Concatenation
        kstring& operator+=(const kstring& other) {
            if (other.empty()) {
                return *this;
            }

            ensure_capacity(_length + other._length + 1);
            memcpy(_data + _length, other._data, other._length + 1);
            _length += other._length;
            return *this;
        }

        kstring& operator+=(const char* s) {
            if (s == nullptr) {
                return *this;
            }

            std::size_t len = strlen(s);
            ensure_capacity(_length + len + 1);
            memcpy(_data + _length, s, len + 1);
            _length += len;
            return *this;
        }

        kstring& operator+=(char c) {
            push_back(c);
            return *this;
        }

        kstring operator+(const kstring& other) const {
            kstring res{*this};
            return res += other;
        }

        kstring operator+(const char* s) const {
            kstring res{*this};
            return res += s;
        }

        // Comparison
        bool operator==(const kstring& other) const {
            if (_length != other._length) {
                return false;
            }
            return memcmp(_data, other._data, _length) == 0;
        }

        bool operator!=(const kstring& other) const {
            return !operator==(other);
        }

        bool operator==(const char* s) const {
            if (s == nullptr) {
                return empty();
            }
            if (empty()) {
                return *s == '\0';
            }
            return strcmp(_data, s) == 0;
        }

        bool operator!=(const char* s) const {
            return !operator==(s);
        }
    };

    inline kstring operator+(const char* lhs, const kstring& rhs) {
        kstring res{lhs};
        return res += rhs;
    }
}
