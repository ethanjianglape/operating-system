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
static process::Process* find_ready_process()
{
    wake_sleeping_processes();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        if (p->state == process::ProcessState::READY || p->state == process::ProcessState::NEW) {
            return p;
        }
    }

    return nullptr;
};

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

        yield_sleep(self, REAP_INTERVAL_MS);
    }

    __builtin_unreachable();
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
    // including the PML4, FS register, TSS.RSP0. And we do not want any anyone
    // else to manipulate these while we are working with them.

    g_processes_lock.lock();

    auto* cpu = arch::percpu::get();
    auto* current = cpu->process;

    process::Process* p = find_ready_process();

    // If there are no processes to schedule, resort to using the
    // idle process on this cpu
    if (p == nullptr) {
        p = cpu->idle_process;
    } else {
        g_processes.rotate_next();
    }

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

    p->state = process::ProcessState::RUNNING;

    cpu->process = p;
    cpu->kernel_rsp = p->kernel_rsp;

    arch::vmm::switch_pml4(p->pml4);
    arch::tls::set_fs_base(p->fs_base);
    arch::gdt::set_kernel_stack(p->kernel_rsp);

    g_processes_lock.unlock();

    context_switch(&current->kernel_rsp_saved, p->kernel_rsp_saved);

    cpu->process = current;
    cpu->kernel_rsp = current->kernel_rsp;

    arch::vmm::switch_pml4(current->pml4);
    arch::tls::set_fs_base(current->fs_base);
    arch::gdt::set_kernel_stack(current->kernel_rsp);
}

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

[[noreturn]]
void yield_dead(process::Process* proc)
{
    auto* per_cpu = arch::percpu::get();

    proc->state = process::ProcessState::DEAD;
    per_cpu->process = nullptr;

    while (true) {
        arch::cpu::cli();
        g_processes_lock.lock();

        process::Process* ready = find_ready_process();

        if (ready != nullptr) {
            ready->state = process::ProcessState::RUNNING;
        }

        bool no_other_processes = g_processes.empty();

        g_processes_lock.unlock();

        if (ready != nullptr) {
            per_cpu->process = ready;
            per_cpu->kernel_rsp = ready->kernel_rsp;

            arch::vmm::switch_pml4(ready->pml4);
            arch::tls::set_fs_base(ready->fs_base);
            arch::gdt::set_kernel_stack(ready->kernel_rsp);

            context_switch(&proc->kernel_rsp_saved, ready->kernel_rsp_saved);

            kpanic("Context switch back to DEAD process");
        } else if (no_other_processes) {
            log::info("========================================");
            log::info("All processes terminated. System halted.");
            log::info("========================================");

            while (true) {
                arch::cpu::cli();
                arch::cpu::hlt();
            }
        } else {
            arch::cpu::sti();
            arch::cpu::hlt();
        }
    }

    __builtin_unreachable();
}

void yield_sleep(process::Process* p, std::uint64_t sleep_time_ms)
{
    p->wake_time_ms = timer::get_ticks() + sleep_time_ms;
    yield_blocked(p, process::WaitReason::SLEEP);
}

void yield_blocked(process::Process* process, process::WaitReason wait_reason)
{
    if (process == nullptr) {
        log::warn("per_cpu->process is null, nothing to yield");
        return;
    }

    auto* per_cpu = arch::percpu::get();

    process->state = process::ProcessState::BLOCKED;
    process->wait_reason = wait_reason;

    while (process->state == process::ProcessState::BLOCKED) {
        arch::cpu::cli();
        g_processes_lock.lock();

        process::Process* ready = find_ready_process();

        // find_ready_process() wakes all sleeping processes that are past
        // their wake time, which could include this very process that is
        // trying to yield itself while sleeping. We do not want to context
        // switch a process to itself, so simply set its state to RUNNING and carry on
        const bool woke_self = ready != nullptr && ready->pid == process->pid;
        const bool can_context_switch = ready != nullptr && !woke_self;

        if (can_context_switch) {
            ready->state = process::ProcessState::RUNNING;
        } else if (woke_self) {
            process->state = process::ProcessState::RUNNING;
        }

        g_processes_lock.unlock();

        if (can_context_switch) {
            per_cpu->process = ready;
            per_cpu->kernel_rsp = ready->kernel_rsp;

            arch::vmm::switch_pml4(ready->pml4);
            arch::tls::set_fs_base(ready->fs_base);
            arch::gdt::set_kernel_stack(ready->kernel_rsp);

            context_switch(&process->kernel_rsp_saved, ready->kernel_rsp_saved);

            per_cpu->process = process;
            per_cpu->kernel_rsp = process->kernel_rsp;

            arch::vmm::switch_pml4(process->pml4);
            arch::tls::set_fs_base(process->fs_base);
            arch::gdt::set_kernel_stack(process->kernel_rsp);
            arch::cpu::sti();
        } else {
            arch::cpu::sti();
            arch::cpu::hlt();
        }
    }
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
