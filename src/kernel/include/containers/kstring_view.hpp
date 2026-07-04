#pragma once

#include <crt/crt.h>

#include <cstddef>
#include <cstdint>

class kstring_view final {
private:
    const char* _data;

    std::size_t _length;

public:
    class const_iterator {
    private:
        const char* _ptr;

    public:
        const_iterator(const char* ptr)
            : _ptr{ptr}
        {
        }
        const char& operator*() const { return *_ptr; }
        const char* operator->() const { return _ptr; }
        const_iterator operator+(int n)
        {
            auto result = *this;
            result._ptr += n;
            return result;
        }
        const_iterator operator-(int n)
        {
            auto result = *this;
            result._ptr -= n;
            return result;
        }
        std::ptrdiff_t operator-(const const_iterator& other)
        {
            auto a = reinterpret_cast<std::uintptr_t>(this->_ptr);
            auto b = reinterpret_cast<std::uintptr_t>(other._ptr);

            return a - b;
        }
        const_iterator& operator++()
        {
            _ptr++;
            return *this;
        }
        const_iterator operator++(int)
        {
            const auto val = *this;
            _ptr++;
            return val;
        }
        const_iterator& operator--()
        {
            _ptr--;
            return *this;
        }
        const_iterator operator--(int)
        {
            const auto val = *this;
            _ptr--;
            return val;
        }
        const_iterator& operator+=(std::size_t n)
        {
            _ptr += n;
            return *this;
        }
        const_iterator& operator-=(std::size_t n)
        {
            _ptr -= n;
            return *this;
        }
        bool operator==(const const_iterator& other) const { return _ptr == other._ptr; }
        bool operator!=(const const_iterator& other) const { return _ptr != other._ptr; }
        bool operator<(const const_iterator& other) const { return _ptr < other._ptr; }
        bool operator>(const const_iterator& other) const { return _ptr > other._ptr; }
        bool operator<=(const const_iterator& other) const { return _ptr <= other._ptr; }
        bool operator>=(const const_iterator& other) const { return _ptr >= other._ptr; }
    };

    static constexpr std::size_t npos = -1;

    kstring_view(const char* str)
        : _data{str}
        , _length{strlen(str)}
    {
    }

    kstring_view(const char* str, std::size_t len)
        : _data{str}
        , _length{len}
    {
    }

    kstring_view(const kstring_view&) = default;
    kstring_view(kstring_view&&) = default;
    kstring_view& operator=(const kstring_view&) = default;
    kstring_view& operator=(kstring_view&&) = default;

    std::size_t length() const { return _length; }
    std::size_t size() const { return _length; }
    bool empty() const { return _length == 0; }

    const_iterator begin() const { return const_iterator{_data}; }
    const_iterator end() const { return const_iterator{_data + _length}; }

    // Element access
    const char& front() const { return _data[0]; }
    const char& back() const { return _data[_length - 1]; }

    const char& operator[](std::size_t i) const { return _data[i]; }

    const char* c_str() const { return _data ? _data : ""; }
    const char* data() const { return _data; }

    std::size_t find(const char* str) const
    {
        if (!str) {
            return npos;
        }

        const std::size_t len = strlen(str);

        if (len == 0 || len > _length) {
            return npos;
        }

        for (std::size_t i = 0; i + len - 1 < _length; i++) {
            bool found = true;

            for (std::size_t j = 0; j < len; j++) {
                if (_data[i + j] != str[j]) {
                    found = false;
                    break;
                }
            }

            if (found) {
                return i;
            }
        }

        return npos;
    }

    kstring_view substr(std::size_t pos, std::size_t len = npos) const
    {
        if (pos >= _length) {
            return "";
        }

        if (len >= _length || len == npos) {
            return kstring_view{_data + pos};
        }

        return kstring_view{_data + pos, len};
    }

    static kstring_view from_userspace(const char* chars);

    friend bool operator==(const kstring_view& lhs, const kstring_view& rhs)
    {
        if (lhs.length() != rhs.length()) {
            return false;
        }

        return memcmp(lhs.data(), rhs.data(), lhs.length()) == 0;
    }

    friend bool operator!=(const kstring_view& lhs, const kstring_view& rhs)
    {
        return !(lhs == rhs);
    }
};
