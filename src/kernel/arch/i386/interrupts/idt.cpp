#include "idt.hpp"

#include "arch/i386/cpu/cpu.hpp"

using namespace i386;

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
    idtr.base = reinterpret_cast<std::uintptr_t>(&idt_entries[0]);
    idtr.limit = sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    for (std::uint32_t vector = 0; vector < IDT_MAX_DESCRIPTORS; vector++) {
        set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    asm volatile("lidt %0" : : "m"(idtr));
    cpu::sti();
}
