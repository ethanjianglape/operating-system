#pragma once

#include <process/process.hpp>

namespace scheduler {
    void init();

    void yield_blocked(process::Process* p);

    void add_process(process::Process* p);
}
