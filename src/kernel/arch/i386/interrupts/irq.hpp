#ifndef ARCH_I386_IRQ_HPP
#define ARCH_I386_IRQ_HPP

#include <cstdint>

namespace i386::irq {
    using irq_handler_t = void (*)(std::uint32_t vector);

    void register_irq_handler(const std::uint32_t vector, irq_handler_t handler);
}

#endif
