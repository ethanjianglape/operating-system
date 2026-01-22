#include <scheduler/scheduler.hpp>
#include <syscall/sys_proc.hpp>
#include <process/process.hpp>
#include <arch.hpp>

namespace syscall {
    int sys_getpid() {
        auto* proc = arch::percpu::current_process();

        return proc->pid;
    }

    int sys_exit(int status) {
        auto* proc = arch::percpu::current_process();

        proc->exit_status = status;

        scheduler::yield_dead(proc);
    }
}
