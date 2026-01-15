#include "process/process.hpp"
#include "scheduler/scheduler.hpp"
#include "timer/timer.hpp"
#include <syscall/sys_sleep.hpp>
#include <arch/x86_64/entry/entry.hpp>

namespace syscall {
    int sys_sleep_ms(std::uint64_t ms) {
        x86_64::entry::PerCPU* per_cpu = x86_64::entry::get_per_cpu();
        process::Process* process = per_cpu->process;

        process->wake_time_ms = timer::get_ticks() + ms;

        scheduler::yield_blocked(process);

        return 0;
    }
}
