#include "exclusive/katomic.hpp"
#include "process/process.hpp"
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
#include <fs/devfs/devfs.hpp>
#include <fs/tmpfs/tmpfs.hpp>
#include <log/log.hpp>
#include <scheduler/scheduler.hpp>
#include <timer/timer.hpp>

#ifdef KERNEL_TESTS
#include <test/test.hpp>
#endif

static int counter1;
static int counter2;
static int counter3;

void kthread1()
{
    while (true) {
        counter1++;
        if (counter1 % 100 == 0) {
            log::debug("kthread1");
        }
        x64::cpu::hlt();
    }
}

void kthread2()
{
    while (true) {
        counter2++;
        if (counter2 % 100 == 0) {
            log::debug("kthread2");
        }

        x64::cpu::hlt();
    }
}

void kthread3()
{
    auto* self = arch::percpu::current_process();

    while (true) {
        counter3++;
        if (counter3 % 100 == 0) {
            log::debug("kthread3");
        }

        self->wake_time_ms = timer::get_ticks() + 10;
        scheduler::yield_blocked(self, process::WaitReason::SLEEP);
    }
}

[[noreturn]]
void kernel_main()
{
    x64::cpu::init();
    x64::percpu::early_init();
    x64::drivers::serial::init();

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

    auto* devfs = new fs::devfs::DevFileSystem{};
    auto* tmpfs = new fs::tmpfs::TmpFileSystem{};

    fs::mount("/dev", devfs, nullptr);
    fs::mount("/tmp", tmpfs, nullptr);

#ifdef KERNEL_TESTS
    test::run_all();
#endif

    console::init();
    fs::devfs::init_tty();
    scheduler::init();

    scheduler::add_process(process::create_kthread(kthread1));
    scheduler::add_process(process::create_kthread(kthread2));
    scheduler::add_process(process::create_kthread(kthread3));

    x64::percpu::enable_preemption();
    x64::cpu::sti();

    while (true) {
        x64::cpu::hlt();
    }
}
