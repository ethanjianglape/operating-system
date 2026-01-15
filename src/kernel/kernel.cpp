#include "fs/devfs/dev_tty.hpp"
#include "fs/devfs/devfs.hpp"
#include "fs/fs.hpp"
#include "fs/initramfs/initramfs.hpp"
#include "process/process.hpp"
#include "scheduler/scheduler.hpp"
#include <arch/x86_64/drivers/keyboard/keyboard.hpp>
#include <cstdint>
#include <arch/x86_64/entry/entry.hpp>
#include <arch/x86_64/percpu/percpu.hpp>
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

#ifdef KERNEL_TESTS
#include <test/test.hpp>
#endif

[[noreturn]]
void kernel_main() {
    x86_64::drivers::serial::init();

    log::info("MyOS Booted into kernel_main() using Limine.");
    log::info("Serial ouput on COM1 initialized");
    
    boot::init();
    
    x86_64::gdt::init();
    x86_64::idt::init();
    x86_64::percpu::init();
    x86_64::entry::init();

    x86_64::drivers::pic::init();
    x86_64::drivers::apic::init();
    x86_64::drivers::keyboard::init();

    x86_64::cpu::sti();

    log::success("All core kernel features initialized!");

#ifdef KERNEL_TESTS
    test::run_all();
#endif

    console::init();
    fs::devfs::tty::init();
    scheduler::init();

    while (true) {
        x86_64::cpu::hlt();
    }
}
