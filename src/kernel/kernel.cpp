#include "arch/i686/gdt/gdt.hpp"
#include "arch/i686/cpu/cpu.hpp"
#include "arch/i686/interrupts/idt.hpp"
#include "arch/i686/syscall/syscall.hpp"
#include "arch/i686/vmm/vmm.hpp"

#include "arch/i686/drivers/pic/pic.hpp"
#include "arch/i686/drivers/vga/vga.hpp"
#include "arch/i686/drivers/apic/apic.hpp"

#include "kernel/kprintf/kprintf.hpp"
#include "kernel/log/log.hpp"
#include "kernel/memory/memory.hpp"
#include <kernel/boot/boot.hpp>

#include <cstdint>

extern "C"
[[noreturn]]
void kernel_main(std::uint32_t multiboot_magic, std::uint32_t multiboot_info_addr) {
    i686::drivers::vga::init();
    kernel::console::init(i686::drivers::vga::get_driver());

    kernel::boot::init(multiboot_magic, multiboot_info_addr);

    //kernel::log::info("Welcome to My OS!");
    //kernel::log::info("multiboot magic = %x", multiboot_magic);
    //kernel::log::info("multiboot info addr = %x", multiboot_info_addr);

    i686::gdt::init();
    i686::vmm::init();
    i686::idt::init();
    i686::syscall::init();
    
    i686::drivers::pic::init();
    i686::drivers::apic::init();

    i686::cpu::sti();

    auto* ptr1 = (std::uint32_t*)kernel::kmalloc(128);
    auto* ptr2 = (std::uint32_t*)kernel::kmalloc(8654);
    auto* ptr3 = (std::uint32_t*)kernel::kmalloc(64);

    kernel::log::info("ptr addr = %x", ptr1);
    kernel::log::info("ptr addr = %x", ptr2);
    kernel::log::info("ptr addr = %x", ptr3);

    //i686::process::init();

    // Infinite loop - kernel_main should never exit
    while (true) {
        i686::cpu::hlt();
    }
}
