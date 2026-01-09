#include "process.hpp"

#include <fmt/fmt.hpp>
#include <log/log.hpp>

using namespace x86_64;

static process::UserspaceFrame frame;

void process::enter_userspace(std::uintptr_t entry, std::uintptr_t stack_top) {
    frame.rip = entry;
    frame.rsp = stack_top;
    frame.ss = 0x1B;
    frame.cs = 0x23;
    frame.eflags = 0x202;

    log::info("****** Entering userspace ******");
    log::info("rip = ", fmt::hex{frame.rip});
    log::info("rsp = ", fmt::hex{frame.rsp});
    log::info("ss  = ", fmt::hex{frame.ss});
    log::info("cs  = ", fmt::hex{frame.cs});
    log::info("f   = ", fmt::bin{frame.eflags});

    asm volatile(
        "mov %0, %%rsp;"
        "iretq;"
        : : "r"(&frame)
     );
}

void process::init() {

}
