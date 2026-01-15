#pragma once

#include <cstdint>

namespace process {
    struct Process;
}

namespace x86_64::percpu {
    constexpr std::uint32_t MSR_GS_BASE        = 0xC0000101;
    constexpr std::uint32_t MSR_KERNEL_GS_BASE = 0xC0000102;

    struct [[gnu::packed]] PerCPU {
        PerCPU* self;
        std::uint64_t kernel_rsp;
        std::uint64_t user_rsp;
        process::Process* process;
    };

    void init();

    PerCPU* get();

    process::Process* current_process();
}
