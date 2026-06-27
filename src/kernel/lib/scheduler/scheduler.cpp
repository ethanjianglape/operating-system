#include "arch/x64/percpu/percpu.hpp"
#include <arch.hpp>
#include <exclusive/kspinlock_irqsave.hpp>
#include <kpanic/kpanic.hpp>
#include <log/log.hpp>
#include <process/process.hpp>
#include <scheduler/scheduler.hpp>
#include <timer/timer.hpp>

#include <cstdint>

namespace scheduler {

static klist<process::Process*> g_processes;
static kspinlock_irqsave g_processes_lock;

extern "C" void context_switch(std::uint64_t* old_rsp_ptr, std::uint64_t new_rsp);

void wake_single(process::WaitReason wait_reason)
{
    g_processes_lock.lock();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        if (p->state == process::ProcessState::BLOCKED && p->wait_reason == wait_reason) {
            p->state = process::ProcessState::READY;
            p->wait_reason = process::WaitReason::NONE;
            p->wake_time_ms = 0;
            goto cleanup;
        }
    }

cleanup:
    g_processes_lock.unlock();
}

void wake_all(process::WaitReason wait_reason)
{
    g_processes_lock.lock();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        if (p->state == process::ProcessState::BLOCKED && p->wait_reason == wait_reason) {
            p->state = process::ProcessState::READY;
            p->wait_reason = process::WaitReason::NONE;
            p->wake_time_ms = 0;
        }
    }

    g_processes_lock.unlock();
}

/// @brief wakes any process that is marked as BLOCKED and has a wake time
/// that is now in the past
static void wake_sleeping_processes()
{
    const std::uintmax_t ticks = timer::get_ticks();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        if (p->state == process::ProcessState::BLOCKED && p->wait_reason == process::WaitReason::SLEEP && p->wake_time_ms > 0 && ticks > p->wake_time_ms) {
            p->state = process::ProcessState::READY;
            p->wait_reason = process::WaitReason::NONE;
            p->wake_time_ms = 0;
        }
    }
}

/// @brief Wakes all sleeping processes then
/// returns the first process that is marked as READY or NEW
static process::Process* select_next_ready_process()
{
    wake_sleeping_processes();

    auto* cpu = arch::percpu::get();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        if (p->state == process::ProcessState::READY || p->state == process::ProcessState::NEW) {
            g_processes.rotate_next();
            return p;
        }
    }

    return cpu->idle_process;
};

static void activate_process(process::Process* p)
{
    auto* cpu = arch::percpu::get();

    p->state = process::ProcessState::RUNNING;

    cpu->process = p;
    cpu->kernel_rsp = p->kernel_rsp;

    arch::vmm::switch_pml4(p->pml4);
    arch::tls::set_fs_base(p->fs_base);
    arch::gdt::set_kernel_stack(p->kernel_rsp);
}

constexpr std::uint64_t REAP_INTERVAL_MS = 50;

static void reaper_kthread()
{
    process::Process* self = arch::percpu::current_process();

    while (true) {
        g_processes_lock.lock();

        for (std::size_t i = 0; i < g_processes.size(); i++) {
            process::Process* proc = g_processes[i];

            if (proc->state != process::ProcessState::DEAD || proc->pid == self->pid) {
                continue;
            }

            process::terminate_process(proc);

            g_processes.erase(i);
        }

        g_processes_lock.unlock();

        yield_sleep(REAP_INTERVAL_MS);
    }

    kpanic("reaper kthread terminated");
}

void preempt()
{
    // Certain sensitive kernel actions, including this preempt() function itself,
    // must disable process preemption to safely perform their actions, so there is
    // nothing to do if preemption has been disabled
    if (!arch::percpu::preemption_enabled()) {
        return;
    }

    // ********************************
    // **** Begin Mutual Exclusion ****
    // ********************************
    //
    // The following section must be performed within a mutually exclusive
    // spinlock that disables both CPU interrupts and preemption
    //
    // We require mutual exclusion because we are directly manipulating the
    // global list of kernel processes (g_processes) and per CPU data fields
    // including the PML4, FS register, and TSS.RSP0. We do not want any anyone
    // else to manipulate these while we are working with them.

    g_processes_lock.lock();

    auto* cpu = arch::percpu::get();
    auto* current = cpu->process;

    process::Process* p = select_next_ready_process();

    // We never want a process to context switch to itself, so we can
    // just leave early if a process wants to switch to itself, after
    // releasing our spinlock of course
    if (p == current) {
        g_processes_lock.unlock();
        return;
    }

    // The process that was interrupted might be blocked, so only set it
    // to ready if it was actively running
    if (current->state == process::ProcessState::RUNNING) {
        current->state = process::ProcessState::READY;
    }

    activate_process(p);

    g_processes_lock.unlock();

    context_switch(&current->kernel_rsp_saved, p->kernel_rsp_saved);
}

[[noreturn]]
void yield_dead()
{
    arch::cpu::cli();
    g_processes_lock.lock();

    auto* cpu = arch::percpu::get();
    auto* current = cpu->process;

    current->state = process::ProcessState::DEAD;

    process::Process* p = select_next_ready_process();

    activate_process(p);

    g_processes_lock.unlock();

    context_switch(&current->kernel_rsp_saved, p->kernel_rsp_saved);

    kpanic("Context switch back to dead process");
}

void yield_sleep(std::uint64_t sleep_time_ms)
{
    arch::percpu::current_process()->wake_time_ms = timer::get_ticks() + sleep_time_ms;
    yield_blocked(process::WaitReason::SLEEP);
}

void yield_blocked(process::WaitReason wait_reason)
{
    g_processes_lock.lock();

    auto* cpu = arch::percpu::get();
    auto* current = cpu->process;

    current->state = process::ProcessState::BLOCKED;
    current->wait_reason = wait_reason;

    process::Process* p = select_next_ready_process();

    // find_ready_process() wakes all sleeping processes that are past
    // their wake time, which could include this very process that is
    // trying to yield itself while sleeping. We do not want to context
    // switch a process to itself, so simply set its state back to RUNNING and carry on
    if (p == current) {
        current->state = process::ProcessState::RUNNING;
        g_processes_lock.unlock();
        return;
    }

    activate_process(p);

    g_processes_lock.unlock();

    context_switch(&current->kernel_rsp_saved, p->kernel_rsp_saved);
}

void add_process(process::Process* p)
{
    g_processes_lock.lock();
    g_processes.push_back(p);
    g_processes_lock.unlock();
}

void init()
{
    log::init_start("Scheduler");
    log::info("Registering schedulers...");

    add_process(process::create_kthread(reaper_kthread));

    log::init_end("Scheduler");
}

}
