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

// Scans once for both jobs: wakes every BLOCKED process whose sleep timer
// has expired (formerly a separate wake_sleeping_processes timer handler —
// folded in here so a sleeping kthread, including reaper_kthread below, can
// be woken without needing its own independent "who wakes the waker"
// mechanism), and returns the first READY/NEW process found. Called from
// every scheduling decision (preempt/yield_blocked/yield_dead), so sleepers
// are checked at least as often as the old per-tick handler did.
static process::Process* find_ready_process()
{
    auto ticks = timer::get_ticks();
    process::Process* ready = nullptr;

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        auto* p = g_processes[i];

        if (p->state == process::ProcessState::BLOCKED && p->wake_time_ms > 0 && ticks > p->wake_time_ms) {
            p->state = process::ProcessState::READY;
            p->wait_reason = process::WaitReason::NONE;
            p->wake_time_ms = 0;
        }

        if (ready == nullptr && (p->state == process::ProcessState::READY || p->state == process::ProcessState::NEW)) {
            ready = p;
        }
    }

    return ready;
};

// Reaps DEAD processes. Used to be a timer handler run inline on every
// tick; now it's a regular kthread, since a process can't free its own
// kernel_stack while still running on it (hence the self-exclusion below)
// and there's no reason this needs to run inside an interrupt context.
constexpr std::uint64_t REAP_INTERVAL_TICKS = 50;

static void reaper_kthread()
{
    auto* self = arch::percpu::current_process();

    while (true) {
        g_processes_lock.lock();

        for (std::size_t i = 0; i < g_processes.size(); i++) {
            auto* proc = g_processes[i];

            if (proc->state != process::ProcessState::DEAD || proc->pid == self->pid) {
                continue;
            }

            process::terminate_process(proc);

            g_processes.erase(i);
        }

        g_processes_lock.unlock();

        self->wake_time_ms = timer::get_ticks() + REAP_INTERVAL_TICKS;
        yield_blocked(self, process::WaitReason::SLEEP);
    }
}

// The single scheduling primitive: every suspension and resumption, whether
// triggered by the timer (here) or cooperatively (yield_blocked/yield_dead),
// goes through context_switch() against kernel_rsp_saved. There is no
// InterruptFrame-mutation path anymore — `current`'s InterruptFrame (pushed
// by this exact timer interrupt) is simply left untouched on its own kernel
// stack. context_switch() may not return until `current` is rescheduled,
// possibly much later, after any number of other processes have run on this
// CPU in between — when it does return, we're back inside this exact call,
// and the rest of this interrupt's call chain (this function -> timer::tick
// -> apic_timer_handler -> the ISR's asm epilogue) unwinds normally,
// iretq-ing through `current`'s original, never-touched InterruptFrame.
//
// This depends on apic_timer_handler sending EOI *before* calling this —
// see the warning there. Called directly from apic_timer_handler rather
// than through timer::register_handler — that mechanism is a plain vector
// of handlers all expected to return promptly within the same tick, which
// this function fundamentally cannot promise, so it isn't allowed to share
// that list.
void preempt()
{
    if (!arch::percpu::preemption_enabled()) {
        return;
    }

    g_processes_lock.lock();

    auto* per_cpu = arch::percpu::get();
    auto* current = per_cpu->process;

    process::Process* p = find_ready_process();

    if (p == nullptr || p == current) {
        // p == current shouldn't normally happen (find_ready_process()
        // doesn't return RUNNING processes), but yield_blocked's own
        // sleep-wake side effect can flip the currently-running process
        // straight to READY without it ever actually stopping — guard
        // against it here too rather than relying solely on every caller
        // to maintain that invariant. Dispatching a process into itself
        // would push a context_switch frame and then jump to its own
        // stale, already-on-the-call-stack resume point — stack corruption.
        g_processes_lock.unlock();
        return;
    }

    g_processes.rotate_next();

    if (current != nullptr && current->state == process::ProcessState::RUNNING) {
        current->state = process::ProcessState::READY;
    }

    p->state = process::ProcessState::RUNNING;

    g_processes_lock.unlock();

    per_cpu->process = p;
    per_cpu->kernel_rsp = p->kernel_rsp;

    arch::vmm::switch_pml4(p->pml4);
    arch::tls::set_fs_base(p->fs_base);

    // current == nullptr means we're preempting out of the idle loop in
    // kernel_main, which has no Process to save into — discard is never
    // targeted by anything, so this context_switch simply never returns.
    std::uint64_t discard;
    context_switch(current != nullptr ? &current->kernel_rsp_saved : &discard, p->kernel_rsp_saved);

    // Resumed: some later preempt/yield_* call switched back to
    // `current`. Per-CPU state may have changed to reflect whatever ran in
    // the meantime, so restore it here — mirrors what yield_blocked does
    // after its own context_switch returns.
    per_cpu->process = current;
    per_cpu->kernel_rsp = current->kernel_rsp;

    arch::vmm::switch_pml4(current->pml4);
    arch::tls::set_fs_base(current->fs_base);
}

void wake_single(process::WaitReason wait_reason)
{
    g_processes_lock.lock();

    for (std::size_t i = 0; i < g_processes.size(); i++) {
        auto* p = g_processes[i];

        if (p->state == process::ProcessState::BLOCKED && p->wait_reason == wait_reason) {
            p->state = process::ProcessState::READY;
            p->wait_reason = process::WaitReason::NONE;
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
        auto* p = g_processes[i];

        if (p->state == process::ProcessState::BLOCKED && p->wait_reason == wait_reason) {
            p->state = process::ProcessState::READY;
            p->wait_reason = process::WaitReason::NONE;
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
        // cli() before locking (not after unlocking) — kspinlock_irqsave's
        // unlock() restores rflags to whatever was captured at lock() time,
        // which would re-enable interrupts here and reopen a window for the
        // timer to nest a preemption into this still-in-progress switch,
        // orphaning `ready` (marked RUNNING but never actually dispatched).
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
}

void yield_blocked(process::Process* process, process::WaitReason wait_reason)
{
    if (process == nullptr) {
        log::warn("per_cpu->process is null, nothing to yield");
        return;
    }

    process->state = process::ProcessState::BLOCKED;
    process->wait_reason = wait_reason;

    while (process->state == process::ProcessState::BLOCKED) {
        // See yield_dead for why cli() happens before locking rather than
        // after unlocking.
        arch::cpu::cli();
        g_processes_lock.lock();

        process::Process* ready = find_ready_process();

        const bool woke_self = ready != nullptr && ready->pid == process->pid;
        const bool can_context_switch = ready != nullptr && !woke_self;

        if (can_context_switch) {
            ready->state = process::ProcessState::RUNNING;
        } else if (woke_self) {
            // find_ready_process()'s sleep-wake side effect can flip
            // `process` itself from BLOCKED straight to READY (it's just
            // another entry in g_processes) before we get a chance to find
            // someone else to switch to. We're refusing to "switch" to
            // ourselves below, so we never actually stopped running —
            // leaving state as READY here would violate "the currently
            // running process's state is RUNNING," which preempt() relies
            // on (it would otherwise see current == p as eligible and try
            // to context_switch a process into itself).
            process->state = process::ProcessState::RUNNING;
        }

        g_processes_lock.unlock();

        if (can_context_switch) {
            auto* per_cpu = arch::percpu::get();
            per_cpu->process = ready;
            per_cpu->kernel_rsp = ready->kernel_rsp;

            arch::vmm::switch_pml4(ready->pml4);
            arch::tls::set_fs_base(ready->fs_base);

            context_switch(&process->kernel_rsp_saved, ready->kernel_rsp_saved);

            per_cpu->process = process;
            per_cpu->kernel_rsp = process->kernel_rsp;

            arch::vmm::switch_pml4(process->pml4);
            arch::tls::set_fs_base(process->fs_base);
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
