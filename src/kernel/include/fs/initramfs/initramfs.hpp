#pragma once

#include <cstddef>
#include <cstdint>

namespace fs::initramfs {
    void init(std::uint8_t* addr, std::size_t size);
}
