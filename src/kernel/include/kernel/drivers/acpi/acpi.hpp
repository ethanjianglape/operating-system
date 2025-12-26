#pragma once

#include <cstdint>
namespace drivers::acpi {
    struct [[gnu::packed]] XSDP {
        char signature[8];
        std::uint8_t checksum;
        char oem_id[6];
        std::uint8_t revision;
        std::uint32_t rsdt_addr; // deprecated in ACPI version 2.0
        std::uint32_t length;
        std::uint64_t xsdt_addr;
        std::uint8_t extended_checksum;
        std::uint8_t reserved[3];
    };

    struct [[gnu::packed]] ACPIHeader {
        char signature[4];
        std::uint32_t length;
        std::uint8_t revision;
        std::uint8_t checksum;
        char oem_id[6];
        char oem_table_id[8];
        std::uint32_t oem_revision;
        std::uint32_t creator_id;
        std::uint32_t creator_revision;
    };

    constexpr std::uintptr_t MADT_RECORD_OFFSET = 0x2C;
    
    constexpr std::uint8_t MADT_LAPIC               = 0;
    constexpr std::uint8_t MADT_IOAPIC              = 1;
    constexpr std::uint8_t MADT_IOAPIC_ISO          = 2; // Interrupt Source Override
    constexpr std::uint8_t MADT_IOAPIC_NIS          = 3; // Non-Maskable Interrupt Source
    constexpr std::uint8_t MADT_LAPIC_NI            = 4; // Non-Maskable Interrupts
    constexpr std::uint8_t MADT_LAPIC_ADDR_OVERRIDE = 5;
    constexpr std::uint8_t MADT_X2APIC              = 9;

    struct [[gnu::packed]] MADTHeader {
        ACPIHeader std_header;
        std::uint32_t lapic_addr;
        std::uint32_t flags;
    };

    struct [[gnu::packed]] XSDT {
        ACPIHeader header;
        std::uint64_t other_headers[];
    };

    void init(void* rsdp_addr);
}
