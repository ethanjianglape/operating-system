#include "process.hpp"

using namespace i386;

[[gnu::aligned(16)]]
static std::uint8_t user_stack[4096];

static process::iret_frame frame;

[[gnu::naked]]
[[gnu::noreturn]]
void user_test_func(void) {
    asm volatile(
        "mov $42, %eax;"
        "int $0x80;"
        "loop: hlt;"
        "jmp loop"
    );
}

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
    /*
    auto func = user_test_func;
    std::uint8_t* stack_top = user_stack + sizeof(user_stack);

    enter_userspace(func, stack_top);
    */
}
