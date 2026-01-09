#pragma once

#include <process/process.hpp>

namespace scheduler {
    void init();

    void add_process(process::Process* p);
}
