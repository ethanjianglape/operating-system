#include <algo/string.hpp>
#include <arch/x86_64/drivers/keyboard/keyboard.hpp>
#include <shell/shell.hpp>
#include <arch/x86_64/syscall/syscall.hpp>
#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/drivers/apic/apic.hpp>
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/interrupts/idt.hpp>
#include <arch/x86_64/drivers/pic/pic.hpp>
#include <arch/x86_64/drivers/serial/serial.hpp>

#include <boot/boot.hpp>
#include <console/console.hpp>
#include <log/log.hpp>
#include <drivers/framebuffer/framebuffer.hpp>

#include <containers/kstring.hpp>

[[noreturn]]
void kernel_main() {
    log::info("MyOS Booted into kernel_main() using Limine.");
    log::info("Serial ouput on COM1 initialized");

    x86_64::drivers::serial::init();

    
    boot::init();
    
    x86_64::gdt::init();
    x86_64::idt::init();
    x86_64::syscall::init();

    x86_64::drivers::pic::init();
    x86_64::drivers::apic::init();
    x86_64::drivers::keyboard::init();

    x86_64::cpu::sti();

    log::success("All core kernel features initialized!");

    shell::init();

    //x86_64::process::init();

    //log::info("back from userspace");

    while (true) {
        x86_64::cpu::hlt();
    }
}
