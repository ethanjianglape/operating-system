/**
 * @file madt.cpp
 * @brief MADT (Multiple APIC Description Table) parser.
 *
 * The MADT describes the system's interrupt controller topology. We need it
 * to find the Local APIC (per-CPU) and I/O APIC (external interrupts) addresses
 * before we can configure interrupts.
 *
 * Structure:
 *
 *   ┌──────────────────────────────────────┐
 *   │ MADT Header                          │
 *   │   - ACPI header (signature "APIC")   │
 *   │   - Local APIC address               │
 *   │   - Flags                            │
 *   ├──────────────────────────────────────┤
 *   │ Record: Local APIC (type 0)          │  ← one per CPU
 *   ├──────────────────────────────────────┤
 *   │ Record: I/O APIC (type 1)            │  ← usually one
 *   ├──────────────────────────────────────┤
 *   │ Record: Interrupt Override (type 2)  │  ← ISA IRQ remaps
 *   ├──────────────────────────────────────┤
 *   │ ...more variable-length records...   │
 *   └──────────────────────────────────────┘
 *
 * Each record has a type and length, allowing us to skip unknown types.
 */

#include "arch/x86_64/memory/vmm.hpp"
#include <cstdint>
#include <acpi/madt.hpp>
#include <arch.hpp>
#include <containers/kvector.hpp>
#include <fmt/fmt.hpp>
#include <log/log.hpp>

namespace acpi::madt {
    // Storage for parsed MADT records (pointers into the memory-mapped MADT)
    //static kvector<const IOApic*> ioapics;
    static kvector<const IOApic*> ioapics;
    static kvector<const InterruptSourceOverride*> overrides;
    static kvector<const LocalApic*> local_apics;
    static kvector<MappedIOApic> mapped_ioapics;
    
    static std::uint64_t lapic_addr;

    std::uint64_t get_lapic_addr() { return lapic_addr; }

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

        local_apics.push_back(lapic);
    }

    static void parse_ioapic(const IOApic* ioapic) {
        log::info("IOAPIC:");
        log::info("  ioapic_id   = ", ioapic->ioapic_id);
        log::info("  ioapic_addr = ", fmt::hex{ioapic->ioapic_addr});
        log::info("  gsi_base    = ", fmt::hex{ioapic->gsi_base});

        std::uintptr_t phys = ioapic->ioapic_addr;
        std::uintptr_t virt = arch::vmm::map_hddm_page(phys, arch::vmm::PAGE_WRITE | arch::vmm::PAGE_CACHE_DISABLE);

        ioapics.push_back(ioapic);
        mapped_ioapics.push_back(MappedIOApic{
            .phys_addr = phys,
            .virt_addr = virt
        });
    }

    static void parse_ioapic_iso(const InterruptSourceOverride* iso) {
        log::info("Interrupt Source Override:");
        log::info("  bus    = ", iso->bus);
        log::info("  source = ", iso->source, " -> gsi = ", iso->gsi);
        log::info("  flags  = ", fmt::bin{iso->flags});

        overrides.push_back(iso);
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

        lapic_addr = override_entry->lapic_addr;
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

        lapic_addr = madt->lapic_addr;

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

    const InterruptSourceOverride* get_override_for_irq(std::uint8_t irq) {
        for (const auto* iso : overrides) {
            if (iso->source == irq) {
                return iso;
            }
        }
        
        return nullptr;
    }

    std::uint32_t get_gsi_for_irq(std::uint8_t irq) {
        const auto* iso = get_override_for_irq(irq);
        
        if (iso != nullptr) {
            return iso->gsi;
        }
        
        // No override - IRQ maps directly to GSI
        return irq;
    }

    const IOApic* get_ioapic_for_gsi(std::uint32_t gsi) {
        constexpr std::uint32_t IOAPIC_MAX_ENTRIES = 24;
        
        for (const auto* ioapic : ioapics) {
            // Each IOAPIC handles 24 GSIs starting from gsi_base
            // (standard IOAPIC has 24 redirection entries)
            if (gsi >= ioapic->gsi_base && gsi < ioapic->gsi_base + IOAPIC_MAX_ENTRIES) {
                return ioapic;
            }
        }
        
        return nullptr;
    }

    volatile std::uint8_t* get_mapped_ioapic_addr(const IOApic* ioapic) {
        std::uintptr_t phys = ioapic->ioapic_addr;

        for (const auto& mapped : mapped_ioapics) {
            if (mapped.phys_addr == phys) {
                return reinterpret_cast<volatile std::uint8_t*>(mapped.virt_addr);
            }
        }

        return nullptr;
    }
}
