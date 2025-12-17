#include "arch/i686/drivers/serial/serial.hpp"
#include "arch/i686/gdt/gdt.hpp"
#include "arch/i686/cpu/cpu.hpp"
#include "arch/i686/interrupts/idt.hpp"
#include "arch/i686/syscall/syscall.hpp"
#include "arch/i686/vmm/vmm.hpp"

#include "arch/i686/drivers/pic/pic.hpp"
#include "arch/i686/drivers/apic/apic.hpp"
#include "kernel/kprintf/kprintf.hpp"

#include <kernel/boot/boot.hpp>
#include <kernel/console/console.hpp>
#include <kernel/log/log.hpp>
#include <kernel/drivers/framebuffer/framebuffer.hpp>

#include <cstdint>

extern "C"
[[noreturn]]
void kernel_main(std::uint32_t multiboot_magic, std::uint32_t multiboot_info_addr) {
    // Serial output to COM1 will be used in the very eary boot process
    // for logging and debugging before anything else is ready
    i686::drivers::serial::init();
    kernel::console::init(i686::drivers::serial::get_console_driver());

    // Init core arch dependencies
    i686::gdt::init();
    i686::vmm::init();
    i686::idt::init();
    i686::syscall::init();

    // Init core arch drivers
    i686::drivers::pic::init();
    i686::drivers::apic::init();

    // Parse the GRUB multiboot2 header to init the PMM and Framebuffer
    kernel::boot::init(multiboot_magic, multiboot_info_addr);

    // Switch to Framebuffer rendering
    kernel::console::init(kernel::drivers::framebuffer::get_console_driver());

    // Enable interrupts to be ready to jump to userspace
    i686::cpu::sti();

    kernel::log::info("Welcome to MyOS!");

    std::uint32_t i = 456;

    int ret = kernel::kprintf2("this is a string %x, %d, %o, %b\n", 100, i, 167, 398);

    kernel::kprintf("printed %x chars\n", ret);

    kernel::log::info("This is an info log");
    kernel::log::success("this is a success");
    kernel::log::warn("this is a warning");
    kernel::log::error("this is an error");
    kernel::log::debug("this is a debug %x", 12345);
    
    // future work, userspace will start here

    // Infinite loop - kernel_main should never exit
    while (true) {
        i686::cpu::hlt();
    }
}
