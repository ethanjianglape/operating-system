#pragma once

#include "log/log.hpp"
#include <arch.hpp>
#include <cstdint>
#include <exclusive/katomic.hpp>

class kspinlock final {
private:
    katomic<int> _counter;
    std::uint64_t _rflags;
    bool _preemption_enabled;

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
    kspinlock()
        : _counter{1}
    {
    }

    ~kspinlock() = default;
    kspinlock(const kspinlock&) = delete;
    kspinlock(kspinlock&&) = delete;
    kspinlock& operator=(const kspinlock&) = delete;
    kspinlock& operator=(kspinlock&&) = delete;

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

        // log::debug("unlocking");

        _counter.store(1);

        if (preemption_enabled) {
            arch::percpu::enable_preemption();
        }

        arch::cpu::write_rflags(rflags);
    }
};
