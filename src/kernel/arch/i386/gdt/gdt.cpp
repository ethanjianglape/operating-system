#include "gdt.hpp"

using namespace i386;

static gdt::gdt_entry gdt_table[6];
static gdt::gdt_ptr gdtr;

extern "C"
void load_gdt(gdt::gdt_ptr* ptr);

gdt::gdt_entry make_entry(std::uint32_t base, std::uint32_t limit, std::uint8_t access, std::uint8_t flags) {
    gdt::gdt_entry entry{};

    // Base address is 32 bits split between 1 16-bit field and 2 8-bit fields
    entry.base_low = base & 0xFFFF;
    entry.base_mid = (base >> 16) & 0xFF;
    entry.base_high = (base >> 24) & 0xFF;

    // Limit is 20 bits split between 1 16-bit field and 1 4-bit field
    entry.limit_low = limit & 0xFFFF;
    entry.limit_mid = (limit >> 16) & 0x0F;

    entry.access = access;

    // Flags is a 4 bit field but can be assigned directly from uint8_t
    entry.flags = flags;

    return entry;
}

void gdt::init() {
    // Entry 0: null descriptor
    gdt_table[0] = {};

    // Entry 1: kernel code
    gdt_table[1] = make_entry(0, 0xFFFFF, KERNEL_CODE, FLAGS_32BIT_4KB);

    // Entry 2: kernel data
    gdt_table[2] = make_entry(0, 0xFFFFF, KERNEL_DATA, FLAGS_32BIT_4KB);

    // Entry 3: user code
    gdt_table[3] = make_entry(0, 0xFFFFF, USER_CODE, FLAGS_32BIT_4KB);

    // Entry 4: user data
    gdt_table[4] = make_entry(0, 0xFFFFF, USER_DATA, FLAGS_32BIT_4KB);

    // Entry 5: TSS (todo later)
    gdt_table[5] = {};

    gdtr.limit = sizeof(gdt_table) - 1;
    gdtr.base = reinterpret_cast<std::uint32_t>(&gdt_table);

    load_gdt(&gdtr);
}
