#include "idt.hpp"

#include "arch/i686/cpu/cpu.hpp"
#include "kernel/log/log.hpp"

using namespace i686;

constexpr int IDT_MAX_DESCRIPTORS = 256;

[[gnu::aligned]]
static idt::idt_entry_t idt_entries[IDT_MAX_DESCRIPTORS];

static idt::idtr_t idtr;

static bool vectors[IDT_MAX_DESCRIPTORS];

extern "C" void* isr_stub_table[];

void set_descriptor(std::uint8_t vector, void* isr, std::uint8_t flags) {
    idt::idt_entry_t* descriptor = &idt_entries[vector];

    descriptor->isr_low = reinterpret_cast<std::uint32_t>(isr) & 0xFFFF;
    descriptor->kernel_cs = 0x08;
    descriptor->attributes = flags;
    descriptor->isr_high = reinterpret_cast<std::uint32_t>(isr) >> 16;
    descriptor->reserved = 0;
}

void idt::init() {
    kernel::log::init_start("IDT");
    
    idtr.base = reinterpret_cast<std::uintptr_t>(&idt_entries[0]);
    idtr.limit = sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    for (std::uint32_t vector = 0; vector < IDT_MAX_DESCRIPTORS; vector++) {
        // By default, DPL=0 (ring0 only)
        std::uint8_t flags = 0x8E;

        // int 0x80 needs to be called by userspace DPL=3 (ring3)
        if (vector == 0x80) {
            flags = 0xEE;
        }
        
        set_descriptor(vector, isr_stub_table[vector], flags);
        vectors[vector] = true;
    }

    asm volatile("lidt %0" : : "m"(idtr));
    cpu::sti();

    kernel::log::init_end("IDT");
}
