#pragma once

#include <cstddef>

#define __user

// Allocates n bytes of memory
void* kmalloc(std::size_t n);

// Allocates num * sizeof(T) bytes of memory
template <typename T>
T* kalloc(std::size_t num)
{
    return reinterpret_cast<T*>(kmalloc(num * sizeof(T)));
}

void kfree(void* ptr);

// Copy size bytes from kernel src into user-space dst. Panics if dst is not a user address.
void kcopy_to_user(void* __user dst, const void* src, std::size_t size);

// Copy size bytes from user-space src into kernel dst. Panics if src is not a user address.
void kcopy_from_user(void* dst, const void* __user src, std::size_t size);
