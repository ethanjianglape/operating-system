#pragma once

#include <acpi/acpi.hpp>
#include <cstdint>

namespace acpi::madt {
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

    struct MappedIOApic {
        std::uintptr_t phys_addr;
        std::uintptr_t virt_addr;
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

    std::uint64_t get_lapic_addr();

    /**
     * @brief Resolves a legacy ISA IRQ to its Global System Interrupt (GSI).
     *
     * Checks the Interrupt Source Override table for remappings. If no override
     * exists, returns the IRQ unchanged (IRQ N = GSI N).
     *
     * @param irq Legacy ISA IRQ number (0-15).
     * @return The corresponding GSI.
     */
    std::uint32_t get_gsi_for_irq(std::uint8_t irq);

    /**
     * @brief Finds the Interrupt Source Override for a given IRQ, if any.
     *
     * @param irq Legacy ISA IRQ number.
     * @return Pointer to the override record, or nullptr if no override exists.
     */
    const InterruptSourceOverride* get_override_for_irq(std::uint8_t irq);

    /**
     * @brief Finds which IOAPIC handles a given GSI.
     *
     * Each IOAPIC has a gsi_base and handles a range of GSIs. This function
     * finds the IOAPIC whose range contains the given GSI.
     *
     * @param gsi The Global System Interrupt number.
     * @return Pointer to the IOApic record, or nullptr if not found.
     */
    const IOApic* get_ioapic_for_gsi(std::uint32_t gsi);

    volatile std::uint8_t* get_mapped_ioapic_addr(const IOApic* ioapic);
}
