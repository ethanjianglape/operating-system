#pragma once

#include <cstddef>

namespace kernel {
    void* kmalloc(std::size_t size);
    
    void kfree(void* ptr);
}
