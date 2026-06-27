#pragma once

#include <arch.hpp>
#include <exclusive/katomic.hpp>

#include <cstdint>

class kspinlock_irqsave final {
private:
    katomic<int> _counter;
    std::uint64_t _rflags;
    bool _preemption_enabled;

    // Performs a test-and-test-and-set loop until the value of _counter
    // switches from 1 to 0
    void spin()
    {
        while (true) {
            while (_counter.load() == 0) {
                arch::cpu::pause();
            }

            if (_counter.exchange(0) == 1) {
                return;
            }
        }
    }

public:
    kspinlock_irqsave()
        : _counter{1}
    {
    }

    ~kspinlock_irqsave() = default;
    kspinlock_irqsave(const kspinlock_irqsave&) = delete;
    kspinlock_irqsave(kspinlock_irqsave&&) = delete;
    kspinlock_irqsave& operator=(const kspinlock_irqsave&) = delete;
    kspinlock_irqsave& operator=(kspinlock_irqsave&&) = delete;

    void lock()
    {
        std::uint64_t rflags = arch::cpu::read_rflags();
        bool preemption_enabled = arch::percpu::preemption_enabled();

        arch::cpu::cli();
        arch::percpu::disable_preemption();

        spin();

        _rflags = rflags;
        _preemption_enabled = preemption_enabled;
    }

    void unlock()
    {
        std::uint64_t rflags = _rflags;
        bool preemption_enabled = _preemption_enabled;

        _counter.store(1);

        if (preemption_enabled) {
            arch::percpu::enable_preemption();
        }

        arch::cpu::write_rflags(rflags);
    }
};
