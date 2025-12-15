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
#include "kernel/memory/memory.hpp"
#include <cstdint>

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

    kernel::pmm::init();

    auto* ptr1 = (std::uint32_t*)kernel::kmalloc(128);
    auto* ptr2 = (std::uint32_t*)kernel::kmalloc(8654);
    auto* ptr3 = (std::uint32_t*)kernel::kmalloc(64);

    kernel::kprintf("kmalloc 1 addr = %x\n", (std::uint32_t)ptr1);
    kernel::kprintf("kmalloc 2 addr = %x\n", (std::uint32_t)ptr2);
    kernel::kprintf("kmalloc 3 addr = %x\n", (std::uint32_t)ptr3);

    ptr1[0] = 0x12345678;
    ptr2[0] = 0xDEADBEEF;
    ptr2[54] = 0xABABABAB;
    ptr2[256] = 123;

    kernel::kprintf("ptr1[0] = %x ptr2[54] = %x\n", ptr1[0], ptr2[54]);

    kernel::log::success("Kernel initialization complete!");
    kernel::log::info("Entering userspace...");

    //i686::process::init();

    // Infinite loop - kernel_main should never exit
    while (true) {
        i686::cpu::hlt();
    }
}
