#include <arch.hpp>
#include <exclusive/kspinlock_irqsave.hpp>
#include <kassert/kassert.hpp>
#include <kpanic/kpanic.hpp>
#include <log/log.hpp>
#include <process/process.hpp>
#include <scheduler/scheduler.hpp>
#include <timer/timer.hpp>

namespace scheduler {

/// @brief finds the next ready process to schedule
///
/// 1. wakes all sleeping processes
/// 2. attempts to find a process that is READY or NEW
/// 3. if found, rotates the queue of processes
/// 4. defaults to the idle process if no process found
///
/// @return pointer to the next ready process
///
process::Process* RoundRobinScheduler::next_ready_process()
{
    wake_sleepers();

    for (std::size_t i = 0; i < _processes.size(); i++) {
        process::Process* p = _processes[i];

        kassert_not_null(p);

        if (p->is_ready()) {
            _processes.rotate_next();
            return p;
        }
    }

    return arch::percpu::idle_process();
};

process::Process* RoundRobinScheduler::find_child(process::Process* parent, int pid)
{
    process::Process* first_match = nullptr;

    for (std::size_t i = 0; i < _processes.size(); i++) {
        process::Process* p = _processes[i];

        if (pid != -1 && p->pid != pid) {
            continue;
        }

        if (p->parent == nullptr || p->parent->pid != parent->pid) {
            continue;
        }

        if (p->is_zombie()) {
            return p;
        }

        if (first_match == nullptr) {
            first_match = p;
        }
    }

    return first_match;
}

}
