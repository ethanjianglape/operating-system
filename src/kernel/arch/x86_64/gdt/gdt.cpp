#include "gdt.hpp"
#include "kernel/log/log.hpp"

using namespace x86_64;

static gdt::GdtTable gdt_table;
static gdt::Gdtr gdtr;
static gdt::TssEntry tss;

[[gnu::aligned(16)]]
static std::uint8_t kernel_stack[16384]; // 16KiB stack

extern "C"
void load_gdt(gdt::Gdtr* ptr);

extern "C"
void load_tss();

gdt::GdtEntry make_gdt_entry(std::uint32_t base, std::uint32_t limit, std::uint8_t access, std::uint8_t flags) {
    gdt::GdtEntry entry{};

    // Base address is 32 bits split between 1 16-bit field and 2 8-bit fields
    entry.base_low = base & 0xFFFF;
    entry.base_mid = (base >> 16) & 0xFF;
    entry.base_high = (base >> 24) & 0xFF;

    // Limit is 20 bits split between 1 16-bit field and 1 4-bit field
    entry.limit_low = limit & 0xFFFF;
    entry.limit_high = (limit >> 16) & 0x0F;

    entry.access = access;

    // Flags is a 4 bit field but can be assigned directly from uint8_t
    entry.flags = flags;

    return entry;
}

gdt::TssDescriptor make_tss_descriptor() {
    gdt::TssDescriptor desc{};

    std::uint64_t base = reinterpret_cast<std::uint64_t>(&tss);
    std::uint32_t limit = sizeof(gdt::TssEntry) - 1;

    desc.limit_low   = limit & 0xFFFF;
    desc.base_low    = base & 0xFFFF;
    desc.base_mid    = (base >> 16) & 0xFF;
    desc.access      = gdt::ACCESS_PRESENT | gdt::ACCESS_RING_0 | gdt::ACCESS_TSS;
    desc.limit_flags = (limit >> 16) & 0x0F;
    desc.base_high   = (base >> 24) & 0xFF;
    desc.base_upper  = (base >> 32) & 0xFFFFFFFF;
    desc.reserved    = 0x0;

    return desc;
}

void init_gdt_table() {
    // Entry 0: null descriptor
    gdt_table.zero = {};

    // Entry 1: kernel code
    gdt_table.kernel_code = make_gdt_entry(0, 0xFFFFF, gdt::KERNEL_CODE, gdt::FLAGS_64BIT_4KB);

    // Entry 2: kernel data
    gdt_table.kernel_data = make_gdt_entry(0, 0xFFFFF, gdt::KERNEL_DATA, gdt::FLAGS_64BIT_4KB);

    // Entry 3: user code
    gdt_table.user_code= make_gdt_entry(0, 0xFFFFF, gdt::USER_CODE, gdt::FLAGS_64BIT_4KB);

    // Entry 4: user data
    gdt_table.user_data = make_gdt_entry(0, 0xFFFFF, gdt::USER_DATA, gdt::FLAGS_64BIT_4KB);

    // Entry 5-6: TSS
    gdt_table.tss = make_tss_descriptor();
}

void init_tss() {
    tss = {};

    tss.rsp0 = reinterpret_cast<std::uint64_t>(kernel_stack) + sizeof(kernel_stack);
    tss.iopb_offset = sizeof(gdt::TssEntry);
}

void log_gdt_entry(gdt::GdtEntry& entry, int i, const char* name) {
    kernel::log::info("GDT[%d]: %s [base (%x,%x,%x) limit (%x,%x) flags (%x) access (%x)]", i, name,
                      entry.base_low, entry.base_mid, entry.base_high,
                      entry.limit_low, entry.limit_high,
                      entry.flags,
                      entry.access);
}

void gdt::init() {
    kernel::log::init_start("GDT");
    
    init_gdt_table();
    init_tss();
    
    gdtr.limit = sizeof(gdt_table) - 1;
    gdtr.base = reinterpret_cast<std::uint64_t>(&gdt_table);

    load_gdt(&gdtr);
    load_tss();

    kernel::log::info("GDT created with 6 entries");
    kernel::log::info("GDT.limit = %x", gdtr.limit);
    kernel::log::info("GDT.base = %x", gdtr.base);

    log_gdt_entry(gdt_table.zero, 0, "NULL");
    log_gdt_entry(gdt_table.kernel_code, 1, "Kernel Code");
    log_gdt_entry(gdt_table.kernel_data, 2, "Kernel Data");
    log_gdt_entry(gdt_table.user_code, 3, "User Code");
    log_gdt_entry(gdt_table.user_data ,4, "User Data");

    //kernel::log::info("GDT[5]: TSS (ss0=%x, esp0=%x)", tss.ss0, tss.rsp0);

    kernel::log::init_end("GDT");
}
