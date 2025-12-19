#include <arch/x86_64/cpu/cpu.hpp>
#include <arch/x86_64/drivers/apic/apic.hpp>
#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/interrupts/idt.hpp>
#include <arch/x86_64/drivers/pic/pic.hpp>

#include <kernel/boot/boot.hpp>
#include <kernel/console/console.hpp>
#include <kernel/log/log.hpp>
#include <kernel/drivers/framebuffer/framebuffer.hpp>

[[noreturn]]
void kernel_main() {
    kernel::boot::init();
    
    x86_64::gdt::init();
    x86_64::idt::init();

    x86_64::drivers::pic::init();
    x86_64::drivers::apic::init();

    kernel::log::success("All core kernel features initialized!");

    x86_64::cpu::sti();

    while (true) {
        x86_64::cpu::hlt();
    }
}
