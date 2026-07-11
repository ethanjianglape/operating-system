#include "arch/x64/percpu/percpu.hpp"
#include "fmt/fmt.hpp"
#include <arch.hpp>
#include <cerrno>
#include <exclusive/kspinlock_irqsave.hpp>
#include <kassert/kassert.hpp>
#include <kpanic/kpanic.hpp>
#include <log/log.hpp>
#include <process/process.hpp>
#include <scheduler/scheduler.hpp>
#include <timer/timer.hpp>

#include <cstdint>

namespace scheduler {

/// global list of all processes in the scheduler
static klist<process::Process*> g_processes;

/// global spinlock used for safety critical segments of the scheduler
static kspinlock_irqsave g_processes_lock;

/// @brief switch execution from one process to another
///
/// @param old_rsp_ptr pointer to the rsp of the previous process
/// @param new_rsp the rsp of the new process
///
/// @note this function is defined in context_switch.s
///
extern "C" void context_switch(std::uint64_t* old_rsp_ptr, std::uint64_t new_rsp);

/// @brief wakes the first processes that is blocked for wait_reason
///
/// @param wait_reason the reason to wake the process
///
void wake_single(process::WaitReason wait_reason)
{
    g_processes_lock.lock();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        kassert_not_null(p);

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

/// @brief wakes every process that is blocked for wait_reason
///
/// @param wait_reason the reason to wake the processes
///
void wake_all(process::WaitReason wait_reason)
{
    g_processes_lock.lock();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        kassert_not_null(p);

        if (p->state == process::ProcessState::BLOCKED && p->wait_reason == wait_reason) {
            p->state = process::ProcessState::READY;
            p->wait_reason = process::WaitReason::NONE;
            p->wake_time_ms = 0;
        }
    }

    g_processes_lock.unlock();
}

static void wake_all_parents(int pid)
{
    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        kassert_not_null(p);

        if (p->state != process::ProcessState::BLOCKED) {
            continue;
        }

        if (p->wait_reason != process::WaitReason::CHILD_PROCESS) {
            continue;
        }

        if (p->wait_pid != pid && p->wait_pid != -1) {
            continue;
        }

        p->state = process::ProcessState::READY;
        p->wait_reason = process::WaitReason::NONE;
        p->wait_pid = -1;
    }
}

/// @brief wake all sleeping processes that have a wake_time_ms in the past
///
static void wake_sleeping_processes()
{
    const std::uintmax_t ticks = timer::get_ticks();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        kassert_not_null(p);

        if (p->state == process::ProcessState::BLOCKED && p->wait_reason == process::WaitReason::SLEEP && p->wake_time_ms > 0 && ticks > p->wake_time_ms) {
            p->state = process::ProcessState::READY;
            p->wait_reason = process::WaitReason::NONE;
            p->wake_time_ms = 0;
        }
    }
}

/// @brief finds the next ready process to schedule
///
/// 1. wakes all sleeping processes
/// 2. attempts to find a process that is READY or NEW
/// 3. if found, rotates the queue of processes
/// 4. defaults to the idle process if no process found
///
/// @return pointer to the next ready process
///
static process::Process* select_next_ready_process()
{
    wake_sleeping_processes();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        kassert_not_null(p);

        if (p->state == process::ProcessState::READY || p->state == process::ProcessState::NEW) {
            g_processes.rotate_next();
            return p;
        }
    }

    return arch::percpu::idle_process();
};

static process::Process* find_child(process::Process* parent, int pid)
{
    process::Process* first_match = nullptr;

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        process::Process* p = g_processes[i];

        if (pid != -1 && p->pid != pid) {
            continue;
        }

        if (p->parent == nullptr || p->parent->pid != parent->pid) {
            continue;
        }

        if (p->state == process::ProcessState::ZOMBIE) {
            return p;
        }

        if (first_match == nullptr) {
            first_match = p;
        }
    }

    return first_match;
}

/// @brief activate a process on the current cpu
///
/// @param p the process to activate
///
static void activate_process(process::Process* p)
{
    auto* cpu = arch::percpu::get();

    kassert_not_null(cpu);
    kassert_not_null(p);

    p->state = process::ProcessState::RUNNING;

    cpu->process = p;
    cpu->kernel_rsp = p->kernel_rsp;

    arch::vmm::switch_pml4(p->pml4);
    arch::tls::set_fs_base(p->fs_base);
    arch::gdt::set_kernel_stack(p->kernel_rsp);
}

constexpr std::uint64_t REAP_INTERVAL_MS = 50;

/// @brief Periodically terminate DEAD processes
///
/// @return this function should never return
///
[[noreturn]]
static void reaper_kthread()
{
    while (true) {
        g_processes_lock.lock();

        process::Process* self = arch::percpu::current_process();

        for (std::size_t i = 0; i < g_processes.size(); i++) {
            process::Process* p = g_processes[i];

            kassert_not_null(p);

            // the reaper_kthread should never attempt to terminate itself,
            // even if it gets marked DEAD for some reason
            if (p == self) {
                continue;
            }

            if (p->state != process::ProcessState::DEAD) {
                continue;
            }

            process::terminate_process(p);

            g_processes.erase(i--);
        }

        g_processes_lock.unlock();

        yield_sleep(REAP_INTERVAL_MS);
    }

    // The reaper kthread should never finish, its responsible for terminating DEAD
    // processes, so if it stopped running, DEAD processes would continue to
    // accumulate, wasting resources
    kpanic("reaper kthread terminated");
}

/// @brief interrupt the current process to schedule a new one
///
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

    process::Process* current = arch::percpu::current_process();
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

/// @brief mark the current process as DEAD and schedule a new one
///
/// @return this function should never return
///
[[noreturn]]
void yield_dead()
{
    arch::cpu::cli();
    g_processes_lock.lock();

    process::Process* current = arch::percpu::current_process();
    current->state = process::ProcessState::DEAD;

    process::Process* p = select_next_ready_process();

    kassert(current != p);
    activate_process(p);
    g_processes_lock.unlock();
    context_switch(&current->kernel_rsp_saved, p->kernel_rsp_saved);

    // A DEAD process should never be the target of a context_switch from another
    // process, because now that this process is marked as DEAD, the reaper_kthread
    // will pick it and terminate it completely, freeing all of the memory it used,
    // so there would be nothing to context_switch back to anyway
    kpanic("Context switch back to dead process");
}

/// mark the current process as ZOMBIE, wake all parents that are
/// waiting on this pid, and schedule a new process
///
/// @return this function should never return
///
[[noreturn]]
void yield_zombie()
{
    arch::cpu::cli();
    g_processes_lock.lock();

    process::Process* current = arch::percpu::current_process();
    current->state = process::ProcessState::ZOMBIE;

    wake_all_parents(current->pid);

    process::Process* p = select_next_ready_process();

    kassert(current != p);
    activate_process(p);
    g_processes_lock.unlock();
    context_switch(&current->kernel_rsp_saved, p->kernel_rsp_saved);

    // A ZOMBIE process should never be the target of context_switch
    kpanic("Context switch back to zombie process");
}

int yield_to_child(int pid)
{
    while (true) {
        g_processes_lock.lock();

        process::Process* parent = arch::percpu::current_process();
        process::Process* child = find_child(parent, pid);

        if (child == nullptr) {
            log::warn("yield_to_child() found no children");
            g_processes_lock.unlock();
            return -ECHILD;
        }

        if (child->state == process::ProcessState::ZOMBIE) {
            child->state = process::ProcessState::DEAD;
            parent->wait_reason = process::WaitReason::NONE;
            parent->wait_pid = -1;
            g_processes_lock.unlock();
            return child->exit_status;
        }

        parent->state = process::ProcessState::BLOCKED;
        parent->wait_reason = process::WaitReason::CHILD_PROCESS;
        parent->wait_pid = pid;

        process::Process* p = select_next_ready_process();

        activate_process(p);
        g_processes_lock.unlock();
        context_switch(&parent->kernel_rsp_saved, p->kernel_rsp_saved);
    }
}

/// @brief put the current process to sleep
///
/// @param sleep_time_ms time in ms to sleep for
///
void yield_sleep(std::uint64_t sleep_time_ms)
{
    process::Process* current = arch::percpu::current_process();
    current->wake_time_ms = timer::get_ticks() + sleep_time_ms;
    yield_blocked(process::WaitReason::SLEEP);
}

/// @brief block the current process and schedule a new one
///
/// @param wait_reason the reason the process is blocked
///
void yield_blocked(process::WaitReason wait_reason)
{
    g_processes_lock.lock();

    process::Process* current = arch::percpu::current_process();

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

/// @brief add a new process to the scheduler
///
/// @param p the process
///
void add_process(process::Process* p)
{
    kassert_not_null(p);

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
