#include "arch/x86_64/vmm/vmm.hpp"
#include "kernel/log/log.hpp"
#include <cstdint>
#include <kernel/arch/arch.hpp>
#include <kernel/drivers/acpi/acpi.hpp>

namespace kernel::drivers::acpi {
    int strnlen(const char* str, int n) {
        int len = 0;

        while (str[len] && len < n) {
            len++;
        }

        return len;
    }
    
    bool strncmp(const char* str1, const char* str2, int n) {
        if (strnlen(str1, n) != strnlen(str2, n)) {
            return false;
        }
        
        for (int i = 0; i < n && str1[i] && str2[i]; i++) {
            if (str1[i] != str2[i]) {
                return false;
            }
        }

        return true;
    }

    void parse_madt(ACPISDTHeader* header) {
        auto* madt = reinterpret_cast<MADTHeader*>(header);
        auto madt_length = madt->std_header.length;

        kernel::log::info("MADT:");
        kernel::log::info("length = %x", madt_length);
        kernel::log::info("local apic addr = %x", madt->lapic_addr);
        kernel::log::info("flags = %b", madt->flags);

        std::uint8_t* madt_start = reinterpret_cast<std::uint8_t*>(madt);
        std::uint8_t* madt_end = madt_start + madt_length;
        std::uint8_t* record_addr = madt_start + MADT_RECORD_OFFSET;

        auto get_uint8_entry = [&record_addr] (std::uint8_t offset) {
            return *(record_addr + offset);
        };

        auto get_uint16_entry = [&record_addr] (std::uint8_t offset) {
            return *(reinterpret_cast<std::uint16_t*>(record_addr + offset));
        };

        auto get_uint32_entry = [&record_addr] (std::uint8_t offset) {
            return *(reinterpret_cast<std::uint32_t*>(record_addr + offset));
        };

        auto get_uint64_entry = [&record_addr] (std::uint8_t offset) {
            return *(reinterpret_cast<std::uint64_t*>(record_addr + offset));
        };

        while (record_addr < madt_end) {
            auto entry_type = *(record_addr + 0);
            auto record_len = *(record_addr + 1);

            switch (entry_type) {
            case MADT_LAPIC: {
                std::uint8_t acpi_id = *(record_addr + 2);
                std::uint8_t apic_id = *(record_addr + 3);
                std::uint8_t flags = *(record_addr + 4);

                kernel::log::info("Processor Local APIC:");
                kernel::log::info("acpi_id = %d, apic_id = %d, flags = %b", acpi_id, apic_id, flags);
                break;
            }
            case MADT_IOAPIC: {
                std::uint8_t ioapic_id = get_uint8_entry(2);
                std::uint32_t ioapic_addr = get_uint32_entry(4);
                std::uint32_t gsi_base = get_uint32_entry(8);

                kernel::log::info("IOAPIC:");
                kernel::log::info("ioapic id = %d, ioapic addr = %x, gsi base = %x", ioapic_id, ioapic_addr, gsi_base);
                break;
            }
            case MADT_IOAPIC_ISO: {
                auto bus_src = get_uint8_entry(2);
                auto irq_src = get_uint8_entry(3);
                auto gsi     = get_uint32_entry(4);
                auto flags   = get_uint16_entry(8);

                kernel::log::info("IOAPIC interrupt Source Override:");
                kernel::log::info("bus = %d, irq = %d, gsi = %d, flags = %b", bus_src, irq_src, gsi, flags);
                break;
            }
            case MADT_IOAPIC_NIS: {
                auto nmi_source = get_uint8_entry(2);
                auto flags = get_uint16_entry(4);
                auto gsi = get_uint32_entry(6);

                kernel::log::info("IOAPIC Non-maskable Interrupt Source:");
                kernel::log::info("NMI Source = %d, Flags = %b, GSI = %d", nmi_source, flags, gsi);
                
                break;
            }
            case MADT_LAPIC_NI: {
                auto acpi_processor_id = get_uint8_entry(2);
                auto flags = get_uint16_entry(3);
                auto lint = get_uint8_entry(5);

                kernel::log::info("LAPIC Non-maskable interrupts:");
                kernel::log::info("ACPI Processor ID = %d, LINT# = %d, Flags = %b", acpi_processor_id, lint, flags);
                
                break;
            }
            case MADT_LAPIC_ADDR_OVERRIDE: {
                auto lapic_addr = get_uint64_entry(4);

                kernel::log::info("LAPIC Address Override:");
                kernel::log::info("LAPIC Address = %x", lapic_addr);
                
                break;
            }
            case MADT_X2APIC: {
                auto x2_apic_id = get_uint32_entry(4);
                auto flags = get_uint32_entry(8);
                auto acpi_id = get_uint32_entry(12);

                kernel::log::info("Processor Local x2APIC:");
                kernel::log::info("Processor local x2APIC ID = %d, ACPI ID = %d, Flags = %b", x2_apic_id, acpi_id, flags);
                
                break;
            }
            }

            record_addr += record_len;
        }
    }
    
    void init(void* rsdp_addr) {
        kernel::log::init_start("ACPI");

        auto* xsdp = reinterpret_cast<XSDP*>(rsdp_addr);
        std::uintptr_t xsdt_phys = reinterpret_cast<std::uintptr_t>(xsdp->xsdt_addr);
        std::uintptr_t xsdt_virt = kernel::arch::vmm::get_hhdm_offset() + xsdt_phys;

        kernel::arch::vmm::map_page(xsdt_virt,
                                    xsdt_phys,
                                    kernel::arch::vmm::PAGE_PRESENT| kernel::arch::vmm::PAGE_CACHE_DISABLE);

        auto* xsdt = reinterpret_cast<XSDT*>(xsdt_virt);

        kernel::log::info("XSDP:");
        kernel::log::info("checksum = %d", xsdp->checksum);
        kernel::log::info("revision = %d", xsdp->revision);
        kernel::log::info("length   = %d", xsdp->length);
        kernel::log::info("xsdt p   = %x", xsdt_phys);
        kernel::log::info("xsdt v   = %x", xsdt_virt);

        int entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;

        char sig[5];

        sig[0] = xsdt->header.signature[0];
        sig[1] = xsdt->header.signature[1];
        sig[2] = xsdt->header.signature[2];
        sig[3] = xsdt->header.signature[3];
        sig[4] = '\0';

        kernel::log::info("XSDT:");
        kernel::log::info("revision = %d", xsdt->header.revision);
        kernel::log::info("length = %d", xsdt->header.length);
        kernel::log::info("other addr = %x", xsdt->other_sdt);
        kernel::log::info("signature = %s", sig);
        kernel::log::info("entries = %d", entries);

        for (int i = 0; i < entries; i++) {
            std::uintptr_t other_sdt_phys = reinterpret_cast<std::uintptr_t>(xsdt->other_sdt[i]);
            std::uintptr_t other_sdt_virt = kernel::arch::vmm::get_hhdm_offset() + other_sdt_phys;

            kernel::arch::vmm::map_page(other_sdt_virt,
                                        other_sdt_phys,
                                        kernel::arch::vmm::PAGE_PRESENT| kernel::arch::vmm::PAGE_CACHE_DISABLE);

            auto* header = reinterpret_cast<ACPISDTHeader*>(other_sdt_virt);
            char sig[5];
            sig[0] = header->signature[0];
            sig[1] = header->signature[1];
            sig[2] = header->signature[2];
            sig[3] = header->signature[3];
            sig[4] = '\0';
            
            kernel::log::info("Entry #%d: type='%s' len=%d", i, (const char*)sig, header->length);

            if (strncmp(header->signature, "APIC", 4)) {
                parse_madt(header);
            }
        }
    }
}
