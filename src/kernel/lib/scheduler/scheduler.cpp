#include "arch/x86_64/memory/vmm.hpp"
#include "arch/x86_64/syscall/syscall.hpp"
#include "fmt/fmt.hpp"

#include <scheduler/scheduler.hpp>
#include <log/log.hpp>
#include <process/process.hpp>
#include <timer/timer.hpp>
#include <arch.hpp>

#include <cstdint>

namespace scheduler {
    static kvector<process::Process*> processes;
    
    void schedule(std::uintmax_t, arch::irq::InterruptFrame* frame) {
        auto* per_cpu = x86_64::syscall::get_per_cpu();

        process::Process* current = per_cpu->process;

        if (current != nullptr) {
            current->state = process::ProcessState::READY;

            // Save CPU state from interrupt frame
            current->rip = frame->rip;
            current->rsp = frame->rsp;
            current->rflags = frame->rflags;
            current->cs = frame->cs;
            current->ss = frame->ss;

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
                if (false) {
                    log::debug("ready process found, pid=", p->pid);
                    log::debug("p->rip    = ", fmt::hex{p->rip});
                    log::debug("p->rsp    = ", fmt::hex{p->rsp});
                    log::debug("p->rflags = ", fmt::hex{p->rflags});
                    log::debug("p->cs     = ", fmt::hex{p->cs});
                    log::debug("p->ss     = ", fmt::hex{p->ss});
                    log::debug("p->rax    = ", fmt::hex{p->rax});
                    log::debug("p->rbx    = ", fmt::hex{p->rbx});
                    log::debug("p->rcx    = ", fmt::hex{p->rcx});
                    log::debug("p->rdx    = ", fmt::hex{p->rdx});
                    log::debug("p->rsi    = ", fmt::hex{p->rsi});
                    log::debug("p->rdi    = ", fmt::hex{p->rdi});
                    log::debug("p->rbp    = ", fmt::hex{p->rbp});
                    log::debug("p->pml4   = ", fmt::hex{p->pml4});
                }

                p->state = process::ProcessState::RUNNING;
                per_cpu->process = p;

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

                arch::vmm::switch_pml4(p->pml4);

                return;
            }
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
