#include "arch/i686/drivers/serial/serial.hpp"
#include "arch/i686/gdt/gdt.hpp"
#include "arch/i686/cpu/cpu.hpp"
#include "arch/i686/interrupts/idt.hpp"
#include "arch/i686/process/process.hpp"
#include "arch/i686/syscall/syscall.hpp"
#include "arch/i686/vmm/vmm.hpp"

#include "arch/i686/drivers/pic/pic.hpp"
#include "arch/i686/drivers/apic/apic.hpp"
#include "kernel/kprintf/kprintf.hpp"

#include <kernel/boot/boot.hpp>
#include <kernel/console/console.hpp>
#include <kernel/drivers/framebuffer/framebuffer.hpp>

#include <cstdint>

extern "C"
[[noreturn]]
void kernel_main(std::uint32_t multiboot_magic, std::uint32_t multiboot_info_addr) {
    i686::drivers::serial::init();
    kernel::console::init(i686::drivers::serial::get_console_driver());
    
    i686::gdt::init();
    i686::idt::init();
    i686::vmm::init();
    i686::syscall::init();
    
    i686::drivers::pic::init();
    i686::drivers::apic::init();

    kernel::boot::init(multiboot_magic, multiboot_info_addr);
    kernel::console::init(kernel::drivers::framebuffer::get_console_driver());

    i686::cpu::sti();

    kernel::kprintf("hello world\n");

    // future work, userspace will start here

    // Infinite loop - kernel_main should never exit
    while (true) {
        i686::cpu::hlt();
    }
}
