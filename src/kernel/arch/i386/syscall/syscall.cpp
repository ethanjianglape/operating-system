#include "syscall.hpp"
#include "arch/i386/interrupts/irq.hpp"
#include "kernel/tty.h"

using namespace i386;

static int sys_write(int fd, const char* buff, std::size_t count) {
    for (std::size_t i = 0; i < count; i++) {
        terminal_putchar(buff[i]);
    }
    
    return count;
}

extern "C"
void syscall_dispatcher(syscall::registers* registers) {
    const auto syscall_num = static_cast<syscall::syscall_number>(registers->eax);

    switch (syscall_num) {
    case syscall::syscall_number::SYS_WRITE: {
        auto fd = registers->ebx;
        auto buf = reinterpret_cast<const char*>(registers->ecx);
        auto count = registers->edx;

        registers->eax = sys_write(fd, buf, count);
        
        break;
    }
    case syscall::syscall_number::TEST:
        terminal_puts("hello from ring3!\n");
        break;
    default:
        registers->eax = syscall::ENOSYS;
        break;
    }
}

void syscall::init() {
    
}
