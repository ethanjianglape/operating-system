#include <stdio.h>

#include <kernel/tty.h>

#include "arch/i386/interrupts/idt.hpp"
#include "arch/i386/pic/pic.hpp"
#include "arch/i386/apic/apic.hpp"
#include "arch/i386/interrupts/irq.hpp"
#include "arch/i386/timers/pit.hpp"

extern "C" [[noreturn]]
void kernel_main(void) {
    i386::idt::init();

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
        i386::apic::timer_init(0, 0);
        printf("APIC initialized!\n");
    } else {
        printf("WARNING: APIC not supported on this CPU!\n");
    }

    // Infinite loop - kernel should never exit
    while (1) {
        asm volatile("hlt");
    }
}
