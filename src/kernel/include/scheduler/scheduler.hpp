#pragma once

#include <process/process.hpp>

namespace scheduler {
    void init();

    [[noreturn]]
    void yield_dead(process::Process* p);
    
    void yield_blocked(process::Process* p);

    void add_process(process::Process* p);
}
