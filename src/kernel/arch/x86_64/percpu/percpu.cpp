#include "percpu.hpp"

#include <arch/x86_64/cpu/cpu.hpp>
#include <process/process.hpp>
#include <log/log.hpp>
#include <fmt/fmt.hpp>

extern constexpr std::size_t PER_CPU_SELF_OFFSET       = offsetof(x86_64::percpu::PerCPU, self);
extern constexpr std::size_t PER_CPU_KERNEL_RSP_OFFSET = offsetof(x86_64::percpu::PerCPU, kernel_rsp);
extern constexpr std::size_t PER_CPU_USER_RSP_OFFSET   = offsetof(x86_64::percpu::PerCPU, user_rsp);
extern constexpr std::size_t PER_CPU_PROCESS_OFFSET    = offsetof(x86_64::percpu::PerCPU, process);

namespace x86_64::percpu {
    void init() {
        log::init_start("PerCPU");

        PerCPU* per_cpu_data = new PerCPU{};

        per_cpu_data->self = per_cpu_data;
        per_cpu_data->kernel_rsp = 0;
        per_cpu_data->user_rsp = 0;
        per_cpu_data->process = nullptr;

        std::uintptr_t gs_base        = reinterpret_cast<std::uintptr_t>(per_cpu_data);
        std::uintptr_t kernel_gs_base = 0;

        cpu::wrmsr(MSR_GS_BASE, gs_base);
        cpu::wrmsr(MSR_KERNEL_GS_BASE, kernel_gs_base);

        log::info("GS_BASE = ", fmt::hex{gs_base});

        log::init_end("PerCPU");
    }

    PerCPU* get() {
        PerCPU* ptr;

        asm volatile("mov %%gs:%c1, %0" : "=r"(ptr) : "i"(PER_CPU_SELF_OFFSET) : "memory");

        return ptr;
    }

    process::Process* current_process() {
        PerCPU* ptr = get();

        return ptr->process;
    }
}
