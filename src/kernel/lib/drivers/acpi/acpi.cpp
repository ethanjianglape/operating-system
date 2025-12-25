#include "arch/x86_64/drivers/apic/apic.hpp"
#include <arch/x86_64/vmm/vmm.hpp>
#include <fmt/fmt.hpp>
#include <kernel/log/log.hpp>
#include <kernel/arch/arch.hpp>
#include <kernel/drivers/acpi/acpi.hpp>

#include <cstdint>

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

    void parse_madt(ACPIHeader* header) {
        auto* madt = reinterpret_cast<MADTHeader*>(header);
        auto madt_length = madt->std_header.length;

        kernel::log::info("MADT:");
        kernel::log::info("length = ", fmt::hex{madt_length});
        kernel::log::info("local apic addr = ", fmt::hex{madt->lapic_addr});
        kernel::log::info("flags = ", fmt::bin{madt->flags});

        std::uint8_t* madt_start = reinterpret_cast<std::uint8_t*>(madt);
        std::uint8_t* madt_end = madt_start + madt_length;
        std::uint8_t* record_addr = madt_start + MADT_RECORD_OFFSET;

        arch::drivers::apic::set_lapic_addr(madt->lapic_addr);

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
            auto entry_type = get_uint8_entry(0);
            auto record_len = get_uint8_entry(1);

            switch (entry_type) {
            case MADT_LAPIC: {
                auto acpi_id = get_uint8_entry(2);
                auto apic_id = get_uint8_entry(3);
                auto flags   = get_uint8_entry(4);

                kernel::log::info("Processor Local APIC:");
                kernel::log::info("acpi_id = ", acpi_id, ", apic_id = ", apic_id, ", flags = ", fmt::bin{flags});
                break;
            }
            case MADT_IOAPIC: {
                auto ioapic_id   = get_uint8_entry(2);
                auto ioapic_addr = get_uint32_entry(4);
                auto gsi_base    = get_uint32_entry(8);

                kernel::log::info("IOAPIC:");
                kernel::log::info("ioapic id = ", ioapic_id, ", ioapic addr = ", fmt::hex{ioapic_addr}, ", gsi base = ", fmt::hex{gsi_base});

                arch::drivers::apic::set_ioapic_addr(ioapic_addr);
                
                break;
            }
            case MADT_IOAPIC_ISO: {
                auto bus_src = get_uint8_entry(2);
                auto irq_src = get_uint8_entry(3);
                auto gsi     = get_uint32_entry(4);
                auto flags   = get_uint16_entry(8);

                kernel::log::info("IOAPIC interrupt Source Override:");
                kernel::log::info("bus = ", bus_src, ", irq = ", irq_src, ", gsi = ", gsi, ", flags = ", fmt::bin{flags});
                break;
            }
            case MADT_IOAPIC_NIS: {
                auto nmi_source = get_uint8_entry(2);
                auto flags = get_uint16_entry(4);
                auto gsi = get_uint32_entry(6);

                kernel::log::info("IOAPIC Non-maskable Interrupt Source:");
                kernel::log::info("NMI Source = ", nmi_source, ", Flags = ", fmt::bin{flags}, ", GSI = ", gsi);
                
                break;
            }
            case MADT_LAPIC_NI: {
                auto acpi_processor_id = get_uint8_entry(2);
                auto flags = get_uint16_entry(3);
                auto lint = get_uint8_entry(5);

                kernel::log::info("LAPIC Non-maskable interrupts:");
                kernel::log::info("ACPI Processor ID = ", acpi_processor_id, ", LINT# = ", lint, ", Flags = ", fmt::bin{flags});
                
                break;
            }
            case MADT_LAPIC_ADDR_OVERRIDE: {
                auto lapic_addr = get_uint64_entry(4);

                kernel::log::info("LAPIC Address Override:");
                kernel::log::info("LAPIC Address = ", fmt::hex{lapic_addr});

                arch::drivers::apic::set_lapic_addr(lapic_addr);
                
                break;
            }
            case MADT_X2APIC: {
                auto x2_apic_id = get_uint32_entry(4);
                auto flags = get_uint32_entry(8);
                auto acpi_id = get_uint32_entry(12);

                kernel::log::info("Processor Local x2APIC:");
                kernel::log::info("Processor local x2APIC ID = ", x2_apic_id, ", ACPI ID = ", acpi_id, ", Flags = ", fmt::bin{flags});
                
                break;
            }
            }

            record_addr += record_len;
        }
    }
    
    void init(void* rsdp_addr) {
        kernel::log::init_start("ACPI");

        auto* xsdp = reinterpret_cast<XSDP*>(rsdp_addr);

        // xsdp->xsdt_addr is a physical address, it must be mapped to our HHDM
        // address space before it can be used.
        std::uintptr_t xsdt_phys = reinterpret_cast<std::uintptr_t>(xsdp->xsdt_addr);
        std::uintptr_t xsdt_virt = arch::vmm::map_hddm_page(xsdt_phys, arch::vmm::PAGE_CACHE_DISABLE);

        auto* xsdt = reinterpret_cast<XSDT*>(xsdt_virt);

        kernel::log::info("XSDP:");
        kernel::log::info("checksum = ", xsdp->checksum);
        kernel::log::info("revision = ", xsdp->revision);
        kernel::log::info("length   = ", xsdp->length);
        kernel::log::info("xsdt p   = ", fmt::hex{xsdt_phys});
        kernel::log::info("xsdt v   = ", fmt::hex{xsdt_virt});

        int entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;

        char sig[5];

        sig[0] = xsdt->header.signature[0];
        sig[1] = xsdt->header.signature[1];
        sig[2] = xsdt->header.signature[2];
        sig[3] = xsdt->header.signature[3];
        sig[4] = '\0';

        kernel::log::info("XSDT:");
        kernel::log::info("revision = ", xsdt->header.revision);
        kernel::log::info("length = ", xsdt->header.length);
        kernel::log::info("signature = ", sig);
        kernel::log::info("entries = ", entries);

        for (int i = 0; i < entries; i++) {
            // Each xsdt->other_headers[i] is a physical pointer to the next entry in the table,
            // it must be mapped to our HHDM address space to access it.
            std::uintptr_t headers_phys = reinterpret_cast<std::uintptr_t>(xsdt->other_headers[i]);
            std::uintptr_t headers_virt = arch::vmm::map_hddm_page(headers_phys, arch::vmm::PAGE_CACHE_DISABLE);

            auto* header = reinterpret_cast<ACPIHeader*>(headers_virt);
            
            char sig[5];
            sig[0] = header->signature[0];
            sig[1] = header->signature[1];
            sig[2] = header->signature[2];
            sig[3] = header->signature[3];
            sig[4] = '\0';
            
            kernel::log::info("Entry #", i, ": type='", (const char*)sig, "' len=", header->length);

            if (strncmp(header->signature, "APIC", 4)) {
                parse_madt(header);
            }
        }
    }
}
