#include "syscall.hpp"
#include "kernel/log/log.hpp"

using namespace x86_64;

static int sys_write(int fd, const char* buff, std::size_t count) {
    for (std::size_t i = 0; i < count; i++) {
        //terminal_putchar(buff[i]);
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
        //terminal_puts("hello from ring3!\n");
        break;
    default:
        registers->eax = syscall::ENOSYS;
        break;
    }
}

void syscall::init() {
    kernel::log::init_start("Syscall at ISR 0x80");
    kernel::log::init_end("Syscall at ISR 0x80");
}
