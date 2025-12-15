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
#include "kernel/panic/panic.hpp"

#include <arch/i686/boot/multiboot2.h>

#include <cstddef>
#include <cstdint>

extern "C"
[[noreturn]]
void kernel_main(std::uint32_t multiboot_magic, std::uint32_t multiboot_info_addr) {
    i686::drivers::vga::init();
    kernel::console::init(i686::drivers::vga::get_driver());

    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kernel::panicf("Multiboot2 incorrect magic value: %x expected %x", multiboot_magic, MULTIBOOT2_BOOTLOADER_MAGIC);
    }

    if (multiboot_info_addr >= 0x02000000) {
        kernel::panicf("Multiboot2 info at %x is outside of kernel low memory space", multiboot_info_addr);
    }

    auto multiboot_size = *(std::uint32_t*)multiboot_info_addr;

    // Iterate tags starting at offset 8
    for (uint8_t* tag = (uint8_t*)(multiboot_info_addr + 8); tag < (uint8_t*)(multiboot_info_addr + multiboot_size); ) {
          uint32_t type = *(uint32_t*)tag;
          uint32_t size = *(uint32_t*)(tag + 4);

          if (type == 0) break;

          if (type == 6) {  // MULTIBOOT_TAG_TYPE_MMAP
              auto* mmap = (multiboot_tag_mmap*)tag;

              std::size_t entry_count = (mmap->size - sizeof(multiboot_tag_mmap)) / mmap->entry_size;
              
              auto* entry = &mmap->entries[0];

              for (std::size_t i =0 ; i < entry_count; i++) {
                  kernel::log::info("Entry %u: addr=0x%x, len=%u, type=%u",
                                    i,
                                    (uint32_t)entry->addr,
                                    (uint32_t)entry->len,
                                    entry->type);

                  entry = (multiboot_mmap_entry*)((uint8_t*)entry + mmap->entry_size);
              }
          }

          tag += (size + 7) & ~7;
      }


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

    // 1GiB to test with
    kernel::pmm::init(1'073'741'824);

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
