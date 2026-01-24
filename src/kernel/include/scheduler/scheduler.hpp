#pragma once

#include <process/process.hpp>

namespace scheduler {
    void init();

    void wake_single(process::WaitReason wait_reason);
    void wake_all(process::WaitReason wait_reason);

    [[noreturn]]
    void yield_dead(process::Process* p);
    
    void yield_blocked(process::Process* p, process::WaitReason wait_reason);

    void add_process(process::Process* p);
}
