#include "arch/x86_64/cpu/cpu.hpp"
#include "fmt/fmt.hpp"
#include <scheduler/scheduler.hpp>
#include <log/log.hpp>
#include <process/process.hpp>
#include <timer/timer.hpp>
#include <arch.hpp>
#include <arch/x86_64/syscall/syscall.hpp>

#include <cstdint>

namespace scheduler {
    static kvector<process::Process*> processes;
    
    void schedule(std::uintmax_t, arch::irq::InterruptFrame* frame) {
        auto* per_cpu = x86_64::syscall::get_per_cpu();

        process::Process* current = per_cpu->process;

        if (current != nullptr) {
            if (frame->cs == 0x08) {
                return;
            }
            
            per_cpu->process = nullptr;

            if (current->state == process::ProcessState::RUNNING) {
                current->state = process::ProcessState::READY;                
            }

            // Save CPU state from interrupt frame
            current->rip = frame->rip;
            current->rsp = frame->rsp;
            current->rflags = frame->rflags;
            current->cs = frame->cs;

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

        for (auto* p : processes) {
            if (p->state == process::ProcessState::READY) {
                p->state = process::ProcessState::RUNNING;
                
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

                frame->r8  = p->r8;
                frame->r9  = p->r9;
                frame->r10 = p->r10;
                frame->r11 = p->r11;
                frame->r12 = p->r12;
                frame->r13 = p->r13;
                frame->r14 = p->r14;
                frame->r15 = p->r15;

                return;
            }
        }
    }

    void yield_blocked(process::Process* process) {
        //auto* per_cpu = x86_64::syscall::get_per_cpu();
        //auto* process = per_cpu->process;

        log::debug("yield process = ", fmt::hex{process});

        if (process == nullptr) {
            log::warn("per_cpu->process is null, nothing to yield");
            return;
        }

        process->state = process::ProcessState::BLOCKED;

        while (process->state == process::ProcessState::BLOCKED) {
            arch::cpu::sti();
            arch::cpu::hlt();
        }
    }

    void add_process(process::Process* p) {
        processes.push_back(p);
    }
    
    void init() {
        log::init_start("Scheduler");

        timer::register_handler(schedule);

        log::init_end("Scheduler");
    }
}
