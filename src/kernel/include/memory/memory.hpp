#pragma once

#include <cstddef>

// Allocates n bytes of memory
void* kmalloc(std::size_t n);

// Allocates num * sizeof(T) bytes of memory
template <typename T>
T* kalloc(std::size_t num) {
    return reinterpret_cast<T*>(kmalloc(num * sizeof(T)));
}

void kfree(void* ptr);
