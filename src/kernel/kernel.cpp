#include "arch/x86_64/process/process.hpp"
#include <arch/x86_64/syscall/syscall.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/drivers/apic/apic.hpp>
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/interrupts/idt.hpp>
#include <arch/x86_64/drivers/pic/pic.hpp>
#include <arch/x86_64/drivers/serial/serial.hpp>

#include <kernel/boot/boot.hpp>
#include <kernel/console/console.hpp>
#include <kernel/log/log.hpp>
#include <kernel/drivers/framebuffer/framebuffer.hpp>

[[noreturn]]
void kernel_main() {
    x86_64::drivers::serial::init();
    kernel::console::init(x86_64::drivers::serial::get_console_driver());

    kernel::log::info("MyOS Booted into kernel_main() using Limine.");
    kernel::log::info("Serial ouput on COM1 initialized");
    
    kernel::boot::init();
    
    x86_64::gdt::init();
    x86_64::idt::init();
    x86_64::syscall::init();

    x86_64::drivers::pic::init();
    x86_64::drivers::apic::init();

    x86_64::cpu::sti();

    kernel::log::success("All core kernel features initialized!");
    kernel::log::info("Entering userspace");

    x86_64::process::init();

    kernel::log::info("back from userspace");

    while (true) {
        x86_64::cpu::hlt();
    }
}
