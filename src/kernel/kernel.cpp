#include "arch/i386/gdt/gdt.hpp"
#include "arch/i386/cpu/cpu.hpp"
#include "arch/i386/interrupts/idt.hpp"
#include "arch/i386/pic/pic.hpp"
#include "arch/i386/apic/apic.hpp"
#include "arch/i386/syscall/syscall.hpp"
#include "arch/i386/paging/paging.hpp"
#include "arch/i386/vga/vga.hpp"

#include "kernel/kprintf/kprintf.hpp"
#include "kernel/log/log.hpp"

extern "C"
[[noreturn]]
void kernel_main(void) {
    i386::vga::init();

    kernel::console::init(i386::vga::putchar,
                          i386::vga::set_color,
                          i386::vga::get_color);
    
    kernel::log::info("Welcome to My OS!");
    kernel::log::info("Beginning initial kernel setup...");

    i386::gdt::init();
    i386::paging::init();
    i386::pic::disable();
    i386::idt::init();
    i386::apic::init();
    i386::apic::timer_init();
    i386::syscall::init();

    kernel::log::success("Kernel initialization complete!");
    kernel::log::info("Entering userspace...");

    //i386::process::init();

    // Infinite loop - kernel_main should never exit
    while (true) {
        i386::cpu::hlt();
    }
}
