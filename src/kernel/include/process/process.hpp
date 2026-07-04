#pragma once

#include "arch/x64/memory/vmm.hpp"
#include <arch.hpp>
#include <containers/kvector.hpp>
#include <fs/fs.hpp>

#include <cstddef>
#include <cstdint>

namespace process {
enum class ProcessState : std::uint8_t {
    NEW = 0,
    RUNNING = 1,
    READY = 2,
    BLOCKED = 3,
    SLEEPING = 4,
    DEAD = 5,
};

enum class WaitReason : std::uint8_t {
    NONE = 0,
    KEYBOARD = 1,
    SLEEP = 2,
    FRAMEBUFFER = 3
};

struct ProcessAllocation {
    std::uintptr_t virt_addr;
    std::size_t num_pages;

    ProcessAllocation(std::uintptr_t addr, std::size_t pages)
        : virt_addr{addr}
        , num_pages{pages}
    {
    }
};

struct Process {
    // Process meta info
    std::size_t pid;
    ProcessState state;
    WaitReason wait_reason;
    int exit_status;

    fs::Inode* cwd_inode;

    // Address space
    arch::vmm::PML4E* pml4;
    std::uintptr_t heap_break;
    std::uintptr_t mmap_min_addr;

    arch::vmm::Heap uheap;

    std::uint8_t* kernel_stack;      // Base of kernel stack
    std::uintptr_t kernel_rsp;       // Top of stack (initially)
    std::uintptr_t kernel_rsp_saved; // The ONLY resume point — every
                                     // suspension (preemptive or
                                     // cooperative) goes through
                                     // context_switch() against this.
    arch::context::ContextFrame* context_frame;

    // VMM page info
    kvector<ProcessAllocation> allocations;

    kvector<void*> uheap_allocations;

    // File Descriptors
    kvector<fs::FileDescriptor*> fd_table;

    // Sleep
    std::uint64_t wake_time_ms;

    std::uintptr_t entry;

    std::uint64_t fs_base; // For thread local storage (TLS)
    int* tidptr;
};

Process* create_process(std::uint8_t* buffer, std::size_t size);

Process* create_kthread(void (*entry)());

void terminate_process(Process* process);
}
