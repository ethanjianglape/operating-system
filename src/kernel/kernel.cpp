#include "arch/i686/gdt/gdt.hpp"
#include "arch/i686/cpu/cpu.hpp"
#include "arch/i686/interrupts/idt.hpp"
#include "arch/i686/syscall/syscall.hpp"
#include "arch/i686/paging/paging.hpp"

#include "arch/i686/drivers/pic/pic.hpp"
#include "arch/i686/drivers/vga/vga.hpp"
#include "arch/i686/drivers/apic/apic.hpp"

#include "kernel/kprintf/kprintf.hpp"
#include "kernel/log/log.hpp"

extern "C"
[[noreturn]]
void kernel_main(void) {
    i686::drivers::vga::init();

    kernel::console::init(i686::drivers::vga::get_driver());

    kernel::log::info("Welcome to My OS!");
    kernel::log::info("Beginning initial kernel setup...");

    i686::gdt::init();
    i686::paging::init();
    i686::idt::init();
    i686::syscall::init();
    
    i686::drivers::pic::init();
    i686::drivers::apic::init();

    i686::cpu::sti();

    kernel::log::success("Kernel initialization complete!");
    kernel::log::info("Entering userspace...");

    //i686::process::init();

    // Infinite loop - kernel_main should never exit
    while (true) {
        i686::cpu::hlt();
    }
}
