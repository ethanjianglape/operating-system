#pragma once

#include <arch.hpp>

namespace syscall {

int sys_getpid();

int sys_fork(arch::trap::SyscallFrame* syscall_frame);

int sys_vfork();

int sys_wait4(int pid, int* wstatus, int options, void* unused);

[[noreturn]]
int sys_exit(int status);

}
