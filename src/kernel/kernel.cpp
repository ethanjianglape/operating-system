#include <stdio.h>

#include <kernel/tty.h>

#include "arch/i386/gdt/gdt.hpp"
#include "arch/i386/cpu/cpu.hpp"
#include "arch/i386/interrupts/idt.hpp"
#include "arch/i386/pic/pic.hpp"
#include "arch/i386/apic/apic.hpp"
#include "arch/i386/syscall/syscall.hpp"
#include "arch/i386/paging/paging.hpp"
#include "arch/i386/process/process.hpp"

extern "C"
[[gnu::noreturn]]
void kernel_main(void) {
    terminal_initialize();
    printf("~~ Welcome to My OS! ~~\n");

    i386::gdt::init();
    i386::paging::init();

    if (i386::pic::disable()) {
        printf("PIC Disabled!\n");
    } else {
        printf("PIC Failed to disable!\n");
    }

    i386::idt::init();

    if (i386::apic::check_support()) {
        printf("APIC supported! Initializing...\n");
        i386::apic::init();
        i386::apic::timer_init();
        printf("APIC initialized!\n");
    } else {
        printf("WARNING: APIC not supported on this CPU!\n");
    }

    printf("INIT syscalls at vector 0x80\n");
    i386::syscall::init();
    printf("done\n");

    //i386::process::init();

    printf("process::init() done.\n");

    // Infinite loop - kernel_main should never exit
    while (true) {
        i386::cpu::hlt();
    }
}
