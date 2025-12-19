/*
#include "arch/x86_64/drivers/serial/serial.hpp"


#include "arch/x86_64/interrupts/idt.hpp"
#include "arch/x86_64/syscall/syscall.hpp"
#include "arch/x86_64/vmm/vmm.hpp"

#include "arch/x86_64/drivers/pic/pic.hpp"
#include "arch/x86_64/drivers/apic/apic.hpp"
 */

#include "arch/x86_64/cpu/cpu.hpp"
#include "arch/x86_64/drivers/serial/serial.hpp"
#include "arch/x86_64/vmm/vmm.hpp"
#include "kernel/kprintf/kprintf.hpp"
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/interrupts/idt.hpp>

#include <kernel/boot/boot.hpp>
#include <kernel/console/console.hpp>
#include <kernel/log/log.hpp>
#include <kernel/drivers/framebuffer/framebuffer.hpp>

#include <cstdint>

[[noreturn]]
void kernel_main() {
    // Serial output to COM1 will be used in the very eary boot process
    // for logging and debugging before anything else is ready
    x86_64::drivers::serial::init();
    kernel::console::init(x86_64::drivers::serial::get_console_driver());

    kernel::log::info("MyOS booted with limine!");

    kernel::boot::init();

    while (true) {
        x86_64::cpu::hlt();
    }

    x86_64::gdt::init();
    x86_64::idt::init();

    //kernel::boot::init(multiboot_magic, multiboot_info_addr);

    x86_64::vmm::init();

    kernel::drivers::framebuffer::init();
    kernel::console::init(kernel::drivers::framebuffer::get_console_driver());

    kernel::log::info("Welcome to MyOS!");
    
    while (true) {
        x86_64::cpu::hlt();
    }

    /*


    // Init core arch dependencies

    x86_64::vmm::init();

    x86_64::syscall::init();

    // Init core arch drivers
    x86_64::drivers::pic::init();
    x86_64::drivers::apic::init();

    // Parse the GRUB multiboot2 header to init the PMM and Framebuffer


    // Switch to Framebuffer rendering
    kernel::console::init(kernel::drivers::framebuffer::get_console_driver());

    // Enable interrupts to be ready to jump to userspace
    x86_64::cpu::sti();

    kernel::log::info("Welcome to MyOS!");

    kernel::drivers::framebuffer::log();

    kernel::log::info("This is an info log");
    kernel::log::success("this is a success");
    kernel::log::warn("this is a warning");
    kernel::log::error("this is an error");
    kernel::log::debug("this is a debug %x", 12345);
    
    // future work, userspace will start here

    // Infinite loop - kernel_main should never exit
    while (true) {
        x86_64::cpu::hlt();
    }
    */
}
