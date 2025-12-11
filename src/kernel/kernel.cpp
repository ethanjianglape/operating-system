#include <stdio.h>

#include <kernel/tty.h>

#include "arch/i386/gdt/gdt.hpp"
#include "arch/i386/cpu/cpu.hpp"
#include "arch/i386/interrupts/idt.hpp"
#include "arch/i386/pic/pic.hpp"
#include "arch/i386/apic/apic.hpp"
#include "arch/i386/interrupts/irq.hpp"
#include "arch/i386/timers/pit.hpp"
#include "arch/i386/syscall/syscall.hpp"
#include "arch/i386/paging/paging.hpp"

extern "C" [[noreturn]]
void kernel_main(void) {
    i386::gdt::init();
    i386::idt::init();
    i386::paging::init();

    terminal_initialize();
    printf("~~ Welcome to My OS! ~~\n");

    // Disable legacy PIC
    if (i386::pic::disable()) {
        printf("PIC Disabled!\n");
    } else {
        printf("PIC Failed to disable!\n");
    }

    // Check APIC support and initialize
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

    asm volatile(
      "mov $1, %%eax\n"      // SYS_WRITE
      "mov $1, %%ebx\n"      // fd = stdout
      "mov %0, %%ecx\n"      // buf = "Test"
      "mov $4, %%edx\n"      // count = 4
      "int $0x80\n"
      : : "r"("Test") : "eax", "ebx", "ecx", "edx"
     );

    // Infinite loop - kernel_main should never exit
    while (true) {
        i386::cpu::hlt();
    }
}
