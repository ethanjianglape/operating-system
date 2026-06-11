#include "process/process.hpp"
#include "scheduler/scheduler.hpp"
#include "timer/timer.hpp"
#include <arch.hpp>
#include <syscall/sys_sleep.hpp>

namespace syscall {
int sys_sleep_ms(std::uint64_t ms)
{
    process::Process* process = arch::percpu::current_process();

    process->wake_time_ms = timer::get_ticks() + ms;

    scheduler::yield_blocked(process, process::WaitReason::SLEEP);

    return 0;
}
}
