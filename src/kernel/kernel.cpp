#include <arch/x64/cpu/cpu.hpp>
#include <arch/x64/drivers/apic/apic.hpp>
#include <arch/x64/drivers/keyboard/keyboard.hpp>
#include <arch/x64/drivers/pic/pic.hpp>
#include <arch/x64/drivers/serial/serial.hpp>
#include <arch/x64/gdt/gdt.hpp>
#include <arch/x64/interrupts/idt.hpp>
#include <arch/x64/percpu/percpu.hpp>
#include <arch/x64/trap/syscall_entry.hpp>

#include <boot/boot.hpp>
#include <console/console.hpp>
#include <containers/kstring.hpp>
#include <framebuffer/framebuffer.hpp>
#include <fs/devfs/dev_tty.hpp>
#include <log/log.hpp>
#include <scheduler/scheduler.hpp>

#ifdef KERNEL_TESTS
#include <test/test.hpp>
#endif

[[noreturn]]
void kernel_main()
{
    x64::drivers::serial::init();
    x64::cpu::init();

    log::info("MyOS Booted into kernel_main() using Limine.");
    log::info("Serial ouput on COM1 initialized");

    boot::init();

    x64::gdt::init();
    x64::idt::init();
    x64::percpu::init();
    x64::trap::init();

    x64::drivers::pic::init();
    x64::drivers::apic::init();
    x64::drivers::keyboard::init();

    log::success("all core kernel features initialized!");

    x64::cpu::dump();

#ifdef KERNEL_TESTS
    test::run_all();
#endif

    x64::cpu::sti();

    console::init();
    fs::devfs::tty::init();
    scheduler::init();

    while (true) {
        x64::cpu::hlt();
    }
}
