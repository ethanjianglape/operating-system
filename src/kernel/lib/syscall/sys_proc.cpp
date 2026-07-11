#include <arch.hpp>
#include <kassert/kassert.hpp>
#include <log/log.hpp>
#include <process/process.hpp>
#include <scheduler/scheduler.hpp>
#include <syscall/sys_proc.hpp>

namespace syscall {

int sys_getpid()
{
    auto* proc = arch::percpu::current_process();

    return proc->pid;
}

int sys_fork(arch::trap::SyscallFrame* syscall_frame)
{
    process::Process* current = arch::percpu::current_process();
    process::Process* created = process::fork_process(current, syscall_frame);

    kassert_not_null(created);

    scheduler::add_process(created);

    return created->pid;
}

int sys_vfork()
{
    log::warn("sys_vfork not implemented");
    return -1;
}

int sys_wait4(int pid, int*, int, void*)
{
    log::debugf("parent pid={} waiting on child pid={}", arch::percpu::current_process()->pid, pid);

    return scheduler::yield_to_child(pid);
}

int sys_exit(int status)
{
    auto* proc = arch::percpu::current_process();

    proc->exit_status = status;

    scheduler::yield_zombie();
}

}
