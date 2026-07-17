#pragma once

#include <arch.hpp>

namespace syscall {

int sys_getpid();

int sys_fork(arch::trap::SyscallFrame* syscall_frame);

int sys_vfork();

int sys_wait4(int pid, int* wstatus, int options, void* unused);

int sys_execve(const char* path, char* const argv[], char* const envp[]);

[[noreturn]]
int sys_exit(int status);

}
