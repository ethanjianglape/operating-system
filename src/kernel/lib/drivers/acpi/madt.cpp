#include <drivers/acpi/madt.hpp>
#include <arch.hpp>
#include <fmt/fmt.hpp>
#include <log/log.hpp>

namespace drivers::acpi::madt {
    static void log_header(const Header* madt) {
        log::info("MADT:");
        log::info("  lapic_addr = ", fmt::hex{madt->lapic_addr});
        log::info("  flags      = ", fmt::bin{madt->flags});
    }

    static void parse_lapic(const LocalApic* lapic) {
        log::info("Local APIC:");
        log::info("  acpi_processor_id = ", lapic->acpi_processor_id);
        log::info("  apic_id           = ", lapic->apic_id);
        log::info("  flags             = ", fmt::bin{lapic->flags});
    }

    static void parse_ioapic(const IOApic* ioapic) {
        log::info("IOAPIC:");
        log::info("  ioapic_id   = ", ioapic->ioapic_id);
        log::info("  ioapic_addr = ", fmt::hex{ioapic->ioapic_addr});
        log::info("  gsi_base    = ", fmt::hex{ioapic->gsi_base});

        arch::drivers::apic::set_ioapic_addr(ioapic->ioapic_addr);
    }

    static void parse_ioapic_iso(const InterruptSourceOverride* iso) {
        log::info("IOAPIC Interrupt Source Override:");
        log::info("  bus    = ", iso->bus);
        log::info("  source = ", iso->source);
        log::info("  gsi    = ", iso->gsi);
        log::info("  flags  = ", fmt::bin{iso->flags});
    }

    static void parse_ioapic_nmi_source(const NMISource* nis) {
        log::info("IOAPIC NMI Source:");
        log::info("  nmi_source = ", nis->nmi_source);
        log::info("  flags      = ", fmt::bin{nis->flags});
        log::info("  gsi        = ", nis->gsi);
    }

    static void parse_lapic_nmi(const LocalApicNMI* nmi) {
        log::info("Local APIC NMI:");
        log::info("  acpi_processor_id = ", nmi->acpi_processor_id);
        log::info("  flags             = ", fmt::bin{nmi->flags});
        log::info("  lint              = ", nmi->lint);
    }

    static void parse_lapic_addr_override(const LocalApicAddressOverride* override_entry) {
        log::info("Local APIC Address Override:");
        log::info("  lapic_addr = ", fmt::hex{override_entry->lapic_addr});

        arch::drivers::apic::set_lapic_addr(override_entry->lapic_addr);
    }

    static void parse_x2apic(const LocalX2Apic* x2apic) {
        log::info("Local x2APIC:");
        log::info("  x2apic_id = ", x2apic->x2apic_id);
        log::info("  flags     = ", fmt::bin{x2apic->flags});
        log::info("  acpi_id   = ", x2apic->acpi_id);
    }

    void parse(ACPIHeader* header) {
        auto* madt = reinterpret_cast<Header*>(header);
        log_header(madt);

        std::uint8_t* madt_start = reinterpret_cast<std::uint8_t*>(madt);
        std::uint8_t* madt_end = madt_start + madt->std_header.length;
        auto* record = reinterpret_cast<RecordHeader*>(madt_start + RECORD_OFFSET);

        arch::drivers::apic::set_lapic_addr(madt->lapic_addr);

        while (reinterpret_cast<std::uint8_t*>(record) < madt_end) {
            switch (record->type) {
            case TYPE_LAPIC:
                parse_lapic(reinterpret_cast<const LocalApic*>(record));
                break;
            case TYPE_IOAPIC:
                parse_ioapic(reinterpret_cast<const IOApic*>(record));
                break;
            case TYPE_IOAPIC_ISO:
                parse_ioapic_iso(reinterpret_cast<const InterruptSourceOverride*>(record));
                break;
            case TYPE_IOAPIC_NMI_SOURCE:
                parse_ioapic_nmi_source(reinterpret_cast<const NMISource*>(record));
                break;
            case TYPE_LAPIC_NMI:
                parse_lapic_nmi(reinterpret_cast<const LocalApicNMI*>(record));
                break;
            case TYPE_LAPIC_ADDR_OVERRIDE:
                parse_lapic_addr_override(reinterpret_cast<const LocalApicAddressOverride*>(record));
                break;
            case TYPE_X2APIC:
                parse_x2apic(reinterpret_cast<const LocalX2Apic*>(record));
                break;
            default:
                log::warn("Unknown MADT record type: ", record->type);
                break;
            }

            record = reinterpret_cast<RecordHeader*>(reinterpret_cast<std::uint8_t*>(record) + record->length);
        }
    }
}
