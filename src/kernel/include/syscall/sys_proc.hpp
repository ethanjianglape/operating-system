#pragma once

namespace syscall {
    int sys_getpid();

    [[noreturn]]
    int sys_exit(int status);
}
