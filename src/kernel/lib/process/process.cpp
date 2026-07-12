#include <arch.hpp>
#include <crt/crt.h>
#include <exclusive/katomic.hpp>
#include <fmt/fmt.hpp>
#include <fs/devfs/dev_tty.hpp>
#include <fs/fs.hpp>
#include <kassert/kassert.hpp>
#include <log/log.hpp>
#include <memory/pmm.hpp>
#include <memory/slab.hpp>
#include <process/elf.hpp>
#include <process/process.hpp>
#include <scheduler/scheduler.hpp>

#include <cstddef>
#include <cstdint>

namespace process {

constexpr std::uintptr_t USER_STACK_BASE = 0x00800000;
constexpr std::uintptr_t USER_STACK_SIZE = 16 * 1024;   // 16KiB
constexpr std::uintptr_t USER_STACK_TOP = USER_STACK_BASE + USER_STACK_SIZE;

constexpr std::uintptr_t KERNEL_STACK_SIZE = 16 * 1024; // 16KiB

/// based on /proc/sys/vm/mmap_min_addr in Linux
constexpr std::uintptr_t DEFAULT_MMAP_MIN_ADDR = 65536;

static katomic<int> g_pid{1};

extern "C" void userspace_entry_trampoline();

extern "C" void forked_entry_trampoline();

static void kthread_entry_trampoline()
{
    // context_switch()'s `ret` never touches rflags — IF here is whatever
    // it was at the call site that dispatched us (e.g. 0 if reached from
    // inside the timer ISR). userspace_entry_trampoline gets this for free
    // via its own iretq; we don't have one, so set it explicitly.
    arch::cpu::sti();

    auto* proc = arch::percpu::current_process();
    auto* entry_func = reinterpret_cast<void (*)()>(proc->entry);

    kassert_not_null(entry_func);

    entry_func();

    scheduler::yield_dead();
}

static void log_user_process(Process* p)
{
    log::debugf("**** User process ****");
    log::debugf("* pid = {}", p->pid);
    log::debugf("* kernel_stack  @ {}", fmt::hex{p->kernel_stack});
    log::debugf("* kernel_rsp    @ {}", fmt::hex{p->kernel_rsp});
    log::debugf("* syscall_frame @ {}", fmt::hex{p->syscall_frame});
    log::debugf("* context_frame @ {}", fmt::hex{p->context_frame});
    log::debugf("* k rsp saved   @ {}", fmt::hex{p->kernel_rsp_saved});
    // log::debugf("* context_frame->r15 = {}", fmt::hex{p->context_frame->r15});
}

Process* create_kthread(void (*entry)())
{
    auto* p = new Process{};

    p->pid = g_pid++;
    p->parent = nullptr;
    p->state = ProcessState::NEW;
    p->wait_reason = WaitReason::NONE;
    p->exit_status = 0;
    p->heap_break = 0;
    p->pml4 = arch::vmm::get_kernel_pml4();
    p->fd_table = {};
    p->kernel_stack = new std::uint8_t[KERNEL_STACK_SIZE];
    p->kernel_rsp = reinterpret_cast<std::uintptr_t>(p->kernel_stack + KERNEL_STACK_SIZE);
    p->wake_time_ms = 0;
    p->mmap_min_addr = DEFAULT_MMAP_MIN_ADDR;
    p->fs_base = 0;
    p->tidptr = 0;
    p->cwd_inode = nullptr;
    p->entry = reinterpret_cast<std::uintptr_t>(entry);

    p->context_frame = reinterpret_cast<arch::context::ContextFrame*>(p->kernel_rsp - sizeof(arch::context::ContextFrame));
    p->context_frame->r15 = 0x15151515; // Magic numbers to help with debugging
    p->context_frame->r14 = 0x14141414;
    p->context_frame->r13 = 0x13131313;
    p->context_frame->r12 = 0x12121212;
    p->context_frame->rbp = 0x77777777;
    p->context_frame->rbx = 0x12345678;
    p->context_frame->rip = reinterpret_cast<std::uintptr_t>(kthread_entry_trampoline);

    p->kernel_rsp_saved = reinterpret_cast<std::uintptr_t>(p->context_frame);

    return p;
}

Process* load_elf(std::uint8_t* buffer, std::size_t size)
{
    elf::Elf64_File file = elf::parse_file(buffer, size);

    if (!file.is_valid_elf) {
        return nullptr;
    }

    arch::vmm::PML4E* pml4 = arch::vmm::create_user_pml4();
    arch::vmm::switch_pml4(pml4);
    arch::cpu::stac();

    auto* p = new Process{};

    p->pid = g_pid++;
    p->state = ProcessState::NEW;
    p->wait_reason = WaitReason::NONE;
    p->exit_status = 0;
    p->heap_break = 0;
    p->pml4 = pml4;
    p->fd_table = {};
    p->kernel_stack = new std::uint8_t[KERNEL_STACK_SIZE];
    p->kernel_rsp = reinterpret_cast<std::uintptr_t>(p->kernel_stack + KERNEL_STACK_SIZE);
    p->wake_time_ms = 0;
    p->mmap_min_addr = DEFAULT_MMAP_MIN_ADDR;
    p->fs_base = 0;
    p->tidptr = 0;
    p->cwd_inode = nullptr;
    p->uheap = arch::vmm::create_user_heap(pml4);

    for (const elf::Elf64_ProgramHeader& header : file.program_headers) {
        auto virt = header.p_vaddr;
        auto file_size = header.p_filesz;
        auto mem_size = header.p_memsz;
        auto offset = header.p_offset;

        log::debugf("mapping user mem at {} len = {}", fmt::hex{virt}, mem_size);

        std::size_t code_pages = arch::vmm::map_pages(pml4, virt, mem_size, arch::vmm::PAGE_USER | arch::vmm::PAGE_WRITE);

        p->allocations.emplace_back(virt & ~0xFFFUL, code_pages);

        memcpy(reinterpret_cast<void*>(virt),
            reinterpret_cast<void*>(buffer + offset),
            file_size);

        if (mem_size > file_size) {
            memset(reinterpret_cast<void*>(virt + file_size), 0, mem_size - file_size);
        }

        std::uintptr_t segment_end = virt + mem_size;

        if (segment_end > p->heap_break) {
            p->heap_break = (segment_end + 0xFFF) & ~0xFFF;
        }
    }

    std::size_t stack_pages = arch::vmm::map_pages(pml4, USER_STACK_BASE, USER_STACK_SIZE, arch::vmm::PAGE_USER | arch::vmm::PAGE_WRITE);

    p->allocations.emplace_back(USER_STACK_BASE, stack_pages);

    // Set up initial stack for Linux ABI compatibility
    // musl libc expects: argc, argv[], NULL, envp[], NULL, auxv[], AT_NULL
    // All zeros: argc=0, argv/envp terminated by NULL, auxv terminated by AT_NULL(0,0)
    // 6 entries = 48 bytes ensures 16-byte alignment (required by System V ABI)
    auto* stack = reinterpret_cast<std::uint64_t*>(USER_STACK_TOP);
    *(--stack) = 0; // padding for 16-byte alignment
    *(--stack) = 0; // AT_NULL value
    *(--stack) = 0; // AT_NULL type
    *(--stack) = 0; // envp terminator (NULL)
    *(--stack) = 0; // argv terminator (NULL)
    *(--stack) = 0; // argc = 0

    p->context_frame = reinterpret_cast<arch::context::ContextFrame*>(p->kernel_rsp - sizeof(arch::context::ContextFrame));
    p->context_frame->r15 = file.entry;
    p->context_frame->r14 = reinterpret_cast<std::uintptr_t>(stack);
    p->context_frame->r13 = 0xDEADBEEF; // Magic numbers to help with debugging
    p->context_frame->r12 = 0xABABABAB;
    p->context_frame->rbp = 0x77777777;
    p->context_frame->rbx = 0x12345678;
    p->context_frame->rip = reinterpret_cast<std::uintptr_t>(userspace_entry_trampoline);
    p->kernel_rsp_saved = reinterpret_cast<std::uintptr_t>(p->context_frame);

    fs::FileDescriptor* stdin = fs::open("/dev/tty1", fs::O_RDONLY);
    fs::FileDescriptor* stdout = fs::open("/dev/tty1", fs::O_WRONLY);
    fs::FileDescriptor* stderr = fs::open("/dev/tty1", fs::O_WRONLY);

    p->fd_table.push_back(stdin);
    p->fd_table.push_back(stdout);
    p->fd_table.push_back(stderr);

    log_user_process(p);

    arch::vmm::switch_kernel_pml4();
    arch::cpu::clac();

    return p;
}

Process* create_process(std::uint8_t* buffer, std::size_t size)
{
    if (buffer == nullptr) {
        log::error("Attempt to load program at NULL");
        return nullptr;
    }

    Process* elf = load_elf(buffer, size);

    return elf;
}

void log_syscall_frame(arch::trap::SyscallFrame* frame)
{
    log::debugf("**** SYSCALL FRAME ****");
    log::debugf("* r15 = {}", fmt::hex{frame->r15});
    log::debugf("* r14 = {}", fmt::hex{frame->r14});
    log::debugf("* r13 = {}", fmt::hex{frame->r13});
    log::debugf("* r12 = {}", fmt::hex{frame->r12});
    log::debugf("* r11 = {}", fmt::hex{frame->r11});
    log::debugf("* r10 = {}", fmt::hex{frame->r10});
    log::debugf("* r9  = {}", fmt::hex{frame->r9});
    log::debugf("* r8  = {}", fmt::hex{frame->r8});
    log::debugf("* rpb = {}", fmt::hex{frame->rbp});
    log::debugf("* rdi = {}", fmt::hex{frame->rdi});
    log::debugf("* rsi = {}", fmt::hex{frame->rsi});
    log::debugf("* rdx = {}", fmt::hex{frame->rdx});
    log::debugf("* rcx = {}", fmt::hex{frame->rcx});
    log::debugf("* rbx = {}", fmt::hex{frame->rbx});
    log::debugf("* rax = {}", fmt::hex{frame->rax});
    log::debugf("* rsp = {}", fmt::hex{frame->rsp});
    log::debugf("***********************");
}

Process* fork_process(Process* current, arch::trap::SyscallFrame* syscall_frame)
{
    kassert_not_null(current);

    arch::vmm::PML4E* pml4 = arch::vmm::clone_user_pml4(current->pml4);
    arch::vmm::switch_pml4(pml4);
    arch::cpu::stac();

    auto* p = new Process{};

    p->pid = g_pid++;
    p->parent = current;
    p->state = process::ProcessState::NEW;
    p->wait_reason = current->wait_reason;
    p->exit_status = current->exit_status;
    p->heap_break = current->heap_break;
    p->pml4 = pml4;
    p->kernel_stack = new std::uint8_t[KERNEL_STACK_SIZE];
    p->kernel_rsp = reinterpret_cast<std::uintptr_t>(p->kernel_stack + KERNEL_STACK_SIZE);
    p->wake_time_ms = current->wake_time_ms;
    p->mmap_min_addr = DEFAULT_MMAP_MIN_ADDR;
    p->fs_base = current->fs_base;
    p->tidptr = current->tidptr;
    p->cwd_inode = current->cwd_inode;
    p->uheap = arch::vmm::clone_user_heap(&current->uheap, pml4);

    p->syscall_frame = reinterpret_cast<arch::trap::SyscallFrame*>(p->kernel_rsp - sizeof(arch::trap::SyscallFrame));

    p->syscall_frame->r15 = syscall_frame->r15;
    p->syscall_frame->r14 = syscall_frame->r14;
    p->syscall_frame->r13 = syscall_frame->r13;
    p->syscall_frame->r12 = syscall_frame->r12;
    p->syscall_frame->r11 = syscall_frame->r11;
    p->syscall_frame->r10 = syscall_frame->r10;
    p->syscall_frame->r9 = syscall_frame->r9;
    p->syscall_frame->r8 = syscall_frame->r8;

    p->syscall_frame->rbp = syscall_frame->rbp;
    p->syscall_frame->rdi = syscall_frame->rdi;
    p->syscall_frame->rsi = syscall_frame->rsi;
    p->syscall_frame->rdx = syscall_frame->rdx;
    p->syscall_frame->rcx = syscall_frame->rcx;
    p->syscall_frame->rbx = syscall_frame->rbx;
    p->syscall_frame->rax = syscall_frame->rax;
    p->syscall_frame->rsp = syscall_frame->rsp;

    log_syscall_frame(p->syscall_frame);

    p->context_frame = reinterpret_cast<arch::context::ContextFrame*>(
        p->kernel_rsp - sizeof(arch::trap::SyscallFrame) - sizeof(arch::context::ContextFrame));
    p->context_frame->r15 = 0x15151515; // Magic numbers to help with debugging
    p->context_frame->r14 = 0x14141414;
    p->context_frame->r13 = 0x13131313;
    p->context_frame->r12 = 0x12121212;
    p->context_frame->rbp = 0xA000000A;
    p->context_frame->rbx = 0xB000000B;
    p->context_frame->rip = reinterpret_cast<std::uintptr_t>(forked_entry_trampoline);
    p->kernel_rsp_saved = reinterpret_cast<std::uintptr_t>(p->context_frame);

    fs::FileDescriptor* stdin = fs::open("/dev/tty1", fs::O_RDONLY);
    fs::FileDescriptor* stdout = fs::open("/dev/tty1", fs::O_WRONLY);
    fs::FileDescriptor* stderr = fs::open("/dev/tty1", fs::O_WRONLY);

    p->fd_table.push_back(stdin);
    p->fd_table.push_back(stdout);
    p->fd_table.push_back(stderr);

    log_user_process(p);

    arch::vmm::switch_pml4(current->pml4);
    arch::cpu::clac();

    return p;
}

bool Process::is_running() const
{
    return state == ProcessState::RUNNING;
}

bool Process::is_ready() const
{
    return state == ProcessState::READY || state == ProcessState::NEW;
}

bool Process::is_zombie() const
{
    return state == ProcessState::ZOMBIE;
}

bool Process::is_dead() const
{
    return state == ProcessState::DEAD;
}

bool Process::is_blocked() const
{
    return state == ProcessState::BLOCKED;
}

bool Process::is_waiting_for(WaitReason reason) const
{
    return wait_reason == reason;
}

bool Process::is_waiting_for_child(int pid) const
{
    return is_waiting_for(WaitReason::CHILD_PROCESS) && (wait_pid == pid || wait_pid == -1);
}

void Process::wake()
{
    state = ProcessState::READY;
    wait_reason = WaitReason::NONE;
    wait_pid = -1;
    wake_time_ms = 0;
}

/// pauses the process if it is currently running
void Process::pause()
{
    if (state == ProcessState::RUNNING) {
        state = ProcessState::READY;
    }
}

void Process::resume()
{
    state = ProcessState::RUNNING;
    wait_reason = WaitReason::NONE;
    wait_pid = -1;
    wake_time_ms = 0;
}

void Process::kill()
{
    state = ProcessState::DEAD;
}

void Process::wait_for(WaitReason reason)
{
    state = ProcessState::BLOCKED;
    wait_reason = reason;
}

void Process::wait_for_child(int child_pid)
{
    wait_for(WaitReason::CHILD_PROCESS);
    wait_pid = child_pid;
}

void Process::sleep_until(std::uint64_t wake_time_ms)
{
    wait_for(WaitReason::SLEEP);
    this->wake_time_ms = wake_time_ms;
}

void Process::terminate()
{
    log::info("========================================");
    log::info("Terminating process ", pid);
    log::info("========================================");

    const auto frames_before = pmm::get_free_frames();
    const auto slabs_before = slab::total_slabs();

    for (fs::FileDescriptor* fd : fd_table) {
        fd->inode->close(fd);
    }

    for (auto& allocation : allocations) {
        arch::vmm::unmap_mem_at(pml4, allocation.virt_addr, allocation.num_pages);
    }

    arch::vmm::free_page_tables(pml4);
    delete[] kernel_stack;

    const auto frames_after = pmm::get_free_frames();
    const auto slabs_after = slab::total_slabs();
    const auto frames_diff = frames_after - frames_before;

    log::infof("PMM frames: {} -> {} (+{})", frames_before, frames_after, frames_diff);
    log::infof("Slabs: {} -> {}", slabs_before, slabs_after);
    log::infof("========================================");
}

Process::~Process()
{
    terminate();
}

}
