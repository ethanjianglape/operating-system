#pragma once

#include <cstdint>

namespace x86_64::irq {
    using irq_handler_t = void (*)(std::uint32_t vector);

    void register_irq_handler(const std::uint32_t vector, irq_handler_t handler);
}

