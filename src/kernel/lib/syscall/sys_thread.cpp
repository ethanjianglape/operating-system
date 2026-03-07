#include <syscall/sys_thread.hpp>
#include <process/process.hpp>
#include <arch.hpp>

namespace syscall {
    long sys_set_tid_address(int* tidptr) {
        auto* proc = arch::percpu::current_process();

        proc->tidptr = tidptr;

        return proc->pid;
    }
}
