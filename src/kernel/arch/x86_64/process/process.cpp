#include "process.hpp"

#include <fmt/fmt.hpp>
#include <log/log.hpp>

using namespace x86_64;

static process::UserspaceFrame frame;

constexpr std::uint32_t USER_CODE_ADDR  = 0x00400000;  // 4MB (low address, us=1)
constexpr std::uint32_t USER_STACK_ADDR = 0x00800000 + 0x00008000; // 5MB (low address, us=1)

[[gnu::naked]]
[[gnu::noreturn]]
void user_test_func(void) {
    asm volatile("mov $1, %%rax\n" // syscall number: write
                 "syscall\n"
                 "hang: jmp hang\n"
                 :
                 :
                 : "rax", "rdi", "rsi", "rdx", "rcx", "r11");
}

void user_test_func_end(void) {}

void enter_userspace(void(*user_code)(), void* user_stack_top) {
    frame.rip = reinterpret_cast<std::uint64_t>(user_code);
    frame.rsp = reinterpret_cast<std::uint64_t>(user_stack_top);
    frame.ss = 0x1B; // User data (0x18 | RPL=3)
    frame.cs = 0x23; // User code (0x20 | RPL=3)
    frame.eflags = 0x202;

    asm volatile(
        "mov %0, %%rsp;"
        "iretq;"
        : : "r"(&frame)
     );
}

void process::init() {
    // Calculate size of user code
    std::uintptr_t start = (uintptr_t)user_test_func;
    std::uintptr_t end = (uintptr_t)user_test_func_end;
    std::size_t code_size = end - start;

    // Copy code from high address to low address
    uint8_t* src = (uint8_t*)start;
    uint8_t* dst = (uint8_t*)USER_CODE_ADDR;

    for (std::size_t i = 0; i < code_size; i++) {
        dst[i] = src[i];
    }

    // Set up user stack at low address
    void* user_stack_top = (void*)USER_STACK_ADDR;

    // Jump to LOW address copy (not high address original!)
    void (*user_entry)() = (void (*)())USER_CODE_ADDR;

    log::debug("Entering userspace at ", fmt::hex{user_entry});
    log::debug("Userspace stack top   ", fmt::hex{user_stack_top});
    log::debug("code size = ", code_size);
    
    enter_userspace(user_entry, user_stack_top);
}
