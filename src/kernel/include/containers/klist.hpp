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
concept klist_storable = requires {
    requires sizeof(T) > 0;
    requires sizeof(T) <= 4096;
    requires std::destructible<T>;
    requires std::move_constructible<T> || std::copy_constructible<T>;
};

template <klist_storable T> class klist final {
private:
    struct node final {
        node* prev;
        node* next;
        T data;
    };

    std::size_t _size;

    node* _head;
public:
    klist() : _head{nullptr}, _size{0} {}

    explicit klist(std::size_t count, const T& value = {}) : klist{} {
        while (count--) {
            push_back(value);
        }
    }

    klist(klist&& other) : _head{other._head}, _size{other._size} {
        other._head = nullptr;
        other._size = 0;
    }

    klist(const klist& other) : klist{} {
        if (other.empty()) {
            return;
        }

        node* n = other._head;

        for (std::size_t i = 0; i < other.size(); i++) {
            push_back(n->data);
            n = n->next;
        }
    }

    klist& operator=(klist&& other) {
        if (this != &other) {
            clear();
            _head = other._head;
            _size = other._size;
            other._head = nullptr;
            other._size = 0;
        }
        
        return *this;
    }

    klist& operator=(const klist& other) {
        if (this != &other) {
            clear();

            node* n = other._head;

            for (std::size_t i = 0; i < other.size(); i++) {
                push_back(n->data);
                n = n->next;
            }
        }
        
        return *this;
    }

    ~klist() {
        clear();
    }

    std::size_t size() const { return _size; }

    bool empty() const { return _size == 0; }

    T& front() { return _head->data; }
    const T& front() const { return _head->data; }

    T& back() { return _head->prev->data; }
    const T& back() const { return _head->prev->data; }

    T& operator[](std::size_t pos) {
        node* n = _head;

        while (pos--) {
            n = n->next;
        }

        return n->data;
    }

    const T& operator[](std::size_t pos) const {
        const node* n = _head;

        while (pos--) {
            n = n->next;
        }

        return n->data;
    }

    void push_front(const T& t) {
        auto* n = new node{};

        if constexpr (std::is_trivially_copyable_v<T>) {
            n->data = t;
        } else {
            new (&n->data) T{t};
        }

        if (_head == nullptr) {
            _head = n;
            _head->next = n;
            _head->prev = n;
        } else {
            n->next = _head;
            n->prev = _head->prev;

            _head->prev->next = n;
            _head->prev = n;
            _head = n;
        }

        _size++;
    }

    void push_back(const T& t) {
        auto* n = new node();

        if constexpr (std::is_trivially_copyable_v<T>) {
            n->data = t;
        } else {
            new (&n->data) T{t};
        }

        if (_head == nullptr) {
            _head = n;
            _head->next = n;
            _head->prev = n;
        } else {
            n->next = _head;
            n->prev = _head->prev;

            _head->prev->next = n;
            _head->prev = n;
        }

        _size++;
    }

    void pop_front() {
        if (empty()) {
            return;
        }

        node* n = _head;

        _size--;

        if (_size == 0 ){
            _head = nullptr;
        } else {
            n->next->prev = n->prev;
            n->prev->next = n->next;

            _head = n->next;
        }

        delete n;
    }

    void pop_back() {
        if (empty()) {
            return;
        }

        node* n = _head->prev;

        _size--;

        if (_size == 0) {
            _head = nullptr;
        } else {
            n->next->prev = n->prev;
            n->prev->next = n->next;
        }

        delete n;
    }

    void clear() {
        while (!empty()) {
            pop_back();
        }
    }
};
