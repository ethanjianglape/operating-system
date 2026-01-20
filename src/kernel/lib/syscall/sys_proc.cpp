#include "arch/x86_64/percpu/percpu.hpp"
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

        proc->state = process::ProcessState::DEAD;
        proc->exit_status = status;

        return 0;
    }
}
