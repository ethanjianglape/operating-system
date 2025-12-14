#include "gdt.hpp"
#include "kernel/log/log.hpp"

using namespace i686;

static gdt::gdt_entry gdt_table[6];
static gdt::gdt_ptr gdtr;
static gdt::tss_entry tss;

[[gnu::aligned(16)]]
static std::uint8_t kernel_stack[16384]; // 16KiB stack

extern "C"
void load_gdt(gdt::gdt_ptr* ptr);

extern "C"
void load_tss(void);

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

void init_gdt_table() {
    // Entry 0: null descriptor
    gdt_table[0] = {};

    // Entry 1: kernel code
    gdt_table[1] = make_entry(0, 0xFFFFF, gdt::KERNEL_CODE, gdt::FLAGS_32BIT_4KB);

    // Entry 2: kernel data
    gdt_table[2] = make_entry(0, 0xFFFFF, gdt::KERNEL_DATA, gdt::FLAGS_32BIT_4KB);

    // Entry 3: user code
    gdt_table[3] = make_entry(0, 0xFFFFF, gdt::USER_CODE, gdt::FLAGS_32BIT_4KB);

    // Entry 4: user data
    gdt_table[4] = make_entry(0, 0xFFFFF, gdt::USER_DATA, gdt::FLAGS_32BIT_4KB);

    // Entry 5: TSS
    std::uint32_t tss_base = reinterpret_cast<std::uint32_t>(&tss);
    std::uint32_t tss_limit = sizeof(gdt::tss_entry) - 1;
    
    gdt_table[5] = make_entry(tss_base, tss_limit, gdt::TSS_ACCESS, 0x00);

}

void init_tss() {
    tss = {};
    tss.esp0 = reinterpret_cast<std::uint32_t>(kernel_stack) + sizeof(kernel_stack);
    tss.ss0 = 0x10;
    tss.iomap_base = sizeof(gdt::tss_entry);
}

void gdt::init() {
    kernel::log::init_start("GDT");
    
    init_gdt_table();
    init_tss();
    
    gdtr.limit = sizeof(gdt_table) - 1;
    gdtr.base = reinterpret_cast<std::uint32_t>(&gdt_table);

    load_gdt(&gdtr);
    load_tss();

    kernel::log::info("GDT created with 6 entries");
    kernel::log::info("GDT[0] = null descriptor");
    kernel::log::info("GDT[1] = kernel code");
    kernel::log::info("GDT[2] = kernel data");
    kernel::log::info("GDT[3] = user code");
    kernel::log::info("GDT[4] = user data");
    kernel::log::info("GDT[5] = TSS (ss0=%u, esp0=%u)", tss.ss0, tss.esp0);

    kernel::log::init_end("GDT");
}
