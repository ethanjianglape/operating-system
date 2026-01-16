#pragma once

#include <drivers/acpi/acpi.hpp>
#include <cstdint>

namespace drivers::acpi::madt {
    constexpr std::uintptr_t RECORD_OFFSET = 0x2C;

    constexpr std::uint8_t TYPE_LAPIC               = 0;
    constexpr std::uint8_t TYPE_IOAPIC              = 1;
    constexpr std::uint8_t TYPE_IOAPIC_ISO          = 2; // Interrupt Source Override
    constexpr std::uint8_t TYPE_IOAPIC_NMI_SOURCE   = 3; // Non-Maskable Interrupt Source
    constexpr std::uint8_t TYPE_LAPIC_NMI           = 4; // Local APIC NMI
    constexpr std::uint8_t TYPE_LAPIC_ADDR_OVERRIDE = 5;
    constexpr std::uint8_t TYPE_X2APIC              = 9;

    struct [[gnu::packed]] RecordHeader {
        std::uint8_t type;
        std::uint8_t length;
    };

    struct [[gnu::packed]] LocalApic {
        RecordHeader header;
        std::uint8_t acpi_processor_id;
        std::uint8_t apic_id;
        std::uint32_t flags;
    };

    struct [[gnu::packed]] IOApic {
        RecordHeader header;
        std::uint8_t ioapic_id;
        std::uint8_t reserved;
        std::uint32_t ioapic_addr;
        std::uint32_t gsi_base;
    };

    struct [[gnu::packed]] InterruptSourceOverride {
        RecordHeader header;
        std::uint8_t bus;
        std::uint8_t source;
        std::uint32_t gsi;
        std::uint16_t flags;
    };

    struct [[gnu::packed]] NMISource {
        RecordHeader header;
        std::uint8_t nmi_source;
        std::uint8_t reserved;
        std::uint16_t flags;
        std::uint32_t gsi;
    };

    struct [[gnu::packed]] LocalApicNMI {
        RecordHeader header;
        std::uint8_t acpi_processor_id;
        std::uint16_t flags;
        std::uint8_t lint;
    };

    struct [[gnu::packed]] LocalApicAddressOverride {
        RecordHeader header;
        std::uint16_t reserved;
        std::uint64_t lapic_addr;
    };

    struct [[gnu::packed]] LocalX2Apic {
        RecordHeader header;
        std::uint16_t reserved;
        std::uint32_t x2apic_id;
        std::uint32_t flags;
        std::uint32_t acpi_id;
    };

    struct [[gnu::packed]] Header {
        ACPIHeader std_header;
        std::uint32_t lapic_addr;
        std::uint32_t flags;
    };

    void parse(ACPIHeader* header);
}
