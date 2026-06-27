#ifdef KERNEL_TESTS

#include <arch.hpp>
#include <exclusive/kspinlock.hpp>
#include <log/log.hpp>
#include <test/test.hpp>

#include <cstdint>

namespace test_kspinlock {

constexpr std::uint64_t IF_BIT = 1ull << 9;

void test_basic_lock_unlock()
{
    kspinlock lock;
    lock.lock();
    lock.unlock();
    test::assert_true(true, "lock() followed by unlock() completes without hanging");
}

void test_relock_after_unlock()
{
    kspinlock lock;

    lock.lock();
    lock.unlock();
    lock.lock();
    lock.unlock();

    test::assert_true(true, "spinlock can be locked and unlocked repeatedly");
}

void test_disables_preemption_while_held()
{
    kspinlock lock;
    lock.lock();
    test::assert_true(!arch::percpu::preemption_enabled(), "preemption is disabled while spinlock is held");
    lock.unlock();
}

void test_restores_preemption_after_unlock()
{
    kspinlock lock;
    bool before = arch::percpu::preemption_enabled();

    lock.lock();
    lock.unlock();

    test::assert_eq(arch::percpu::preemption_enabled(), before, "preemption is restored to its prior state after unlock");
}

void test_does_not_disable_interrupts()
{
    kspinlock lock;
    std::uint64_t before = arch::cpu::read_rflags() & IF_BIT;

    lock.lock();
    std::uint64_t during = arch::cpu::read_rflags() & IF_BIT;
    lock.unlock();

    test::assert_eq(during, before, "kspinlock does not touch the interrupt flag while held");
}

void test_preserves_already_disabled_preemption()
{
    bool was_enabled = arch::percpu::preemption_enabled();

    arch::percpu::disable_preemption();

    kspinlock lock;
    lock.lock();
    lock.unlock();

    test::assert_true(!arch::percpu::preemption_enabled(),
        "unlock does not re-enable preemption that was already disabled before lock()");

    if (was_enabled) {
        arch::percpu::enable_preemption();
    }
}

void run()
{
    log::info("Running kspinlock tests...");

    test_basic_lock_unlock();
    test_relock_after_unlock();
    test_disables_preemption_while_held();
    test_restores_preemption_after_unlock();
    test_does_not_disable_interrupts();
    test_preserves_already_disabled_preemption();
}
}

#endif // KERNEL_TESTS
