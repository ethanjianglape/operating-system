#pragma once

#include <cstdint>
namespace acpi {
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

    constexpr const char* SIG_MADT = "APIC";

    struct [[gnu::packed]] XSDT {
        ACPIHeader header;
        std::uint64_t other_headers[];
    };

    void init(void* rsdp_addr);
}
