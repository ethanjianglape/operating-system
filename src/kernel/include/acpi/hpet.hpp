#pragma once

#include "acpi/acpi.hpp"
namespace acpi::hpet {

struct [[gnu::packed]] AddressStructure {
    std::uint8_t address_space_id; // 0 - system memory, 1 - system I/O
    std::uint8_t register_bit_width;
    std::uint8_t register_bit_offset;
    std::uint8_t reserved;
    std::uint64_t address;
};

struct [[gnu::packed]] HPET : public ACPIHeader {
    std::uint8_t hardware_rev_id;
    std::uint8_t comparator_count   : 5;
    std::uint8_t counter_size       : 1;
    std::uint8_t reserved           : 1;
    std::uint8_t legacy_replacement : 1;
    std::uint16_t pci_vendor_id;
    AddressStructure address;
    std::uint8_t hpet_number;
    std::uint16_t minimum_tick;
    std::uint8_t page_protection;
};

void parse(ACPIHeader* header);

}
