#include "kpanic/kpanic.hpp"
#include <scheduler/scheduler.hpp>
#include <log/log.hpp>
#include <process/process.hpp>
#include <timer/timer.hpp>
#include <arch.hpp>

#include <cstdint>

namespace scheduler {
    static klist<process::Process*> g_processes;

    extern "C" void context_switch(std::uint64_t* old_rsp_ptr, std::uint64_t new_rsp);

    static void wake_sleeping_processes(std::uintmax_t ticks, arch::irq::InterruptFrame*) {
        for (std::size_t i = 0; i < g_processes.size(); i++) {
            auto* proc = g_processes[i];
            
            if (proc->state == process::ProcessState::BLOCKED && proc->wake_time_ms > 0 && ticks > proc->wake_time_ms) {
                proc->state = process::ProcessState::READY;
                proc->wait_reason = process::WaitReason::NONE;
                proc->wake_time_ms = 0;
            }
        }
    }

    static void terminate_dead_processes(std::uintmax_t, arch::irq::InterruptFrame*) {
        auto* current_proc = arch::percpu::current_process();
        
        for (std::size_t i = 0; i < g_processes.size(); i++) {
            auto* proc = g_processes[i];

            if (proc->state != process::ProcessState::DEAD || (current_proc && proc->pid == current_proc->pid)) {
                continue;
            }

            process::terminate_process(proc);

            g_processes.erase(i);
        }
    }

    static process::Process* find_ready_kernel_process() {
        for (std::size_t i = 0; i < g_processes.size(); i++) {
            auto* p = g_processes[i];

            if (p->state == process::ProcessState::READY && p->has_kernel_context) {
                return p;
            }
        }

        return nullptr;
    };

    static process::Process* find_ready_user_process() {
        for (std::size_t i = 0; i < g_processes.size(); i++) {
            auto* p = g_processes[i];
            
            if (p->state == process::ProcessState::READY && p->has_user_context) {
                return p;
            }
        }

        return nullptr;
    };

    static void schedule(std::uintmax_t, arch::irq::InterruptFrame* frame) {
        auto* per_cpu = arch::percpu::get();
        auto* current = per_cpu->process;

        if (current != nullptr) {
            if (frame->cs == 0x08) {
                return;
            }
            
            per_cpu->process = nullptr;

            if (current->state == process::ProcessState::RUNNING) {
                current->state = process::ProcessState::READY;                
            }

            current->has_kernel_context = false;
            current->has_user_context = true;

            // Save CPU state from interrupt frame
            current->rip = frame->rip;
            current->rsp = frame->rsp;
            current->rflags = frame->rflags;

            // General purpose registers
            current->rax = frame->rax;
            current->rbx = frame->rbx;
            current->rcx = frame->rcx;
            current->rdx = frame->rdx;
            current->rsi = frame->rsi;
            current->rdi = frame->rdi;
            current->rbp = frame->rbp;

            current->r8  = frame->r8;
            current->r9  = frame->r9;
            current->r10 = frame->r10;
            current->r11 = frame->r11;
            current->r12 = frame->r12;
            current->r13 = frame->r13;
            current->r14 = frame->r14;
            current->r15 = frame->r15;
        }

        process::Process* p = find_ready_user_process();

        if (p != nullptr) {
            g_processes.rotate_next();
            
            p->state = process::ProcessState::RUNNING;
            p->has_kernel_context = false;
            p->has_user_context = true;

            per_cpu->process = p;
            per_cpu->kernel_rsp = p->kernel_rsp;

            arch::vmm::switch_pml4(p->pml4);

            // Restore CPU state to interrupt frame
            frame->rip = p->rip;
            frame->rsp = p->rsp;
            frame->rflags = p->rflags;
            frame->cs = p->cs;
            frame->ss = p->ss;

            // General purpose registers
            frame->rax = p->rax;
            frame->rbx = p->rbx;
            frame->rcx = p->rcx;
            frame->rdx = p->rdx;
            frame->rsi = p->rsi;
            frame->rdi = p->rdi;
            frame->rbp = p->rbp;

            frame->r8 = p->r8;
            frame->r9 = p->r9;
            frame->r10 = p->r10;
            frame->r11 = p->r11;
            frame->r12 = p->r12;
            frame->r13 = p->r13;
            frame->r14 = p->r14;
            frame->r15 = p->r15;
        }
    }

    void wake_single(process::WaitReason wait_reason) {
        arch::cpu::cli();

        for (std::size_t i = 0; i < g_processes.size(); i++) {
            auto* p = g_processes[i];

            if (p->state == process::ProcessState::BLOCKED && p->wait_reason == wait_reason) {
                p->state = process::ProcessState::READY;
                p->wait_reason = process::WaitReason::NONE;
                goto cleanup;
            }
        }

 cleanup:
        arch::cpu::sti();
    }

    void wake_all(process::WaitReason wait_reason) {
        arch::cpu::cli();
        
        for (std::size_t i = 0; i < g_processes.size(); i++) {
            auto* p = g_processes[i];

            if (p->state == process::ProcessState::BLOCKED && p->wait_reason == wait_reason) {
                p->state = process::ProcessState::READY;
                p->wait_reason = process::WaitReason::NONE;
            }
        }

        arch::cpu::sti();
    }

    void yield_dead(process::Process* proc) {
        auto* per_cpu = arch::percpu::get();

        proc->state = process::ProcessState::DEAD;
        per_cpu->process = nullptr;

        while (true) {
            process::Process* ready = find_ready_kernel_process();

            if (ready == nullptr) {
                ready = find_ready_user_process();
            }

            if (ready != nullptr) {
                per_cpu->process = ready;
                per_cpu->kernel_rsp = ready->kernel_rsp;

                arch::vmm::switch_pml4(ready->pml4);

                ready->state = process::ProcessState::RUNNING;

                arch::cpu::sti();

                context_switch(&proc->kernel_rsp_saved, ready->kernel_rsp_saved);

                kpanic("Context switch back to DEAD process pid ", proc->pid);
            } else if (g_processes.empty()) {
                log::info("========================================");
                log::info("All processes terminated. System halted.");
                log::info("========================================");
                
                while (true) {
                    arch::cpu::cli();
                    arch::cpu::hlt();
                }
            } else {
                arch::cpu::sti();
                arch::cpu::hlt();
            }
        }
    }

    void yield_blocked(process::Process* process, process::WaitReason wait_reason) {
        if (process == nullptr) {
            log::warn("per_cpu->process is null, nothing to yield");
            return;
        }

        process->state = process::ProcessState::BLOCKED;
        process->wait_reason = wait_reason;

        while (process->state == process::ProcessState::BLOCKED) {
            process::Process* ready = find_ready_kernel_process();

            if (ready == nullptr) {
                ready = find_ready_user_process();
            }
            
            if (ready != nullptr) {
                // Switch to the target process's page tables and set up per_cpu
                arch::cpu::cli();

                auto* per_cpu = arch::percpu::get();
                per_cpu->process = ready;
                per_cpu->kernel_rsp = ready->kernel_rsp;
                
                arch::vmm::switch_pml4(ready->pml4);

                ready->state = process::ProcessState::RUNNING;
                
                context_switch(&process->kernel_rsp_saved, ready->kernel_rsp_saved);

                per_cpu->process = process;
                per_cpu->kernel_rsp = process->kernel_rsp;

                arch::vmm::switch_pml4(process->pml4);
                arch::cpu::sti();
            } else {
                arch::cpu::sti();
                arch::cpu::hlt();
            }
        }
    }

    void add_process(process::Process* p) {
        g_processes.push_back(p);
    }

    void init() {
        timer::register_handler(wake_sleeping_processes);
        timer::register_handler(terminate_dead_processes);
        timer::register_handler(schedule);
    }
}
