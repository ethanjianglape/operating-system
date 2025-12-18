#include "process.hpp"

using namespace x86_64;

[[gnu::aligned(16)]]
static std::uint8_t user_stack[4096];

static process::iret_frame frame;

constexpr std::uint32_t USER_CODE_ADDR = 0x00100000;  // 4MB (low address, us=1)
constexpr std::uint32_t USER_STACK_ADDR = 0x00200000; // 5MB (low address, us=1)

[[gnu::naked]]
[[gnu::noreturn]]
void user_test_func(void) {
     asm volatile(
      "mov $42, %%eax;"    // Syscall number 42
      "int $0x80;"         // Syscall!
      "1: jmp 1b"          // Loop forever
      : : : "eax"
  );
}

void user_test_func_end(void) {}

void enter_userspace(void(*user_code)(), void* user_stack_top) {
    frame.eip = reinterpret_cast<std::uint32_t>(user_code);
    frame.cs = 0x1B;
    frame.eflags = 0x202;
    frame.esp = reinterpret_cast<std::uint32_t>(user_stack_top);
    frame.ss = 0x23;

    asm volatile(
        "mov %0, %%esp;"
        "iret;"
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
    enter_userspace(user_entry, user_stack_top);
}
