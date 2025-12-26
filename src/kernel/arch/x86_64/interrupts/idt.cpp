#include "idt.hpp"

#include <log/log.hpp>
#include <cstdint>

extern "C" void* isr_stub_table[];

namespace x86_64::idt {
    constexpr int IDT_MAX_DESCRIPTORS = 256;

    alignas(16)
    static IdtEntry idt_entries[IDT_MAX_DESCRIPTORS];

    alignas(16)
    static Idtr idtr;

    void set_descriptor(std::uint8_t vector, void* isr_ptr, std::uint8_t ist, std::uint8_t flags) {
        IdtEntry* desc = &idt_entries[vector];

        const auto isr = reinterpret_cast<std::uintptr_t>(isr_ptr);

        desc->offset_low = isr & 0xFFFF;
        desc->offset_mid = (isr >> 16) & 0xFFFF;
        desc->offset_high = (isr >> 32) & 0xFFFFFFFF;
        desc->ist = ist & 0x7; // only bits 0-2 used
        desc->selector = 0x08;
        desc->attributes = flags;
        desc->reserved = 0x0;
    }

    void init() {
        log::init_start("IDT");

        idtr.base = reinterpret_cast<std::uint64_t>(&idt_entries[0]);
        idtr.limit = sizeof(IdtEntry) * IDT_MAX_DESCRIPTORS - 1;

        for (std::uint32_t vector = 0; vector < IDT_MAX_DESCRIPTORS; vector++) {
            // By default, DPL=0 (ring0 only)
            std::uint8_t flags = 0x8E;
            std::uint8_t ist = 0x0;

            // int 0x80 needs to be called by userspace DPL=3 (ring3)
            if (vector == 0x80) {
                flags = 0xEE;
            }

            set_descriptor(vector, isr_stub_table[vector], ist, flags);
        }

        asm volatile("lidt %0" : : "m"(idtr));

        log::init_end("IDT");
    }
}

