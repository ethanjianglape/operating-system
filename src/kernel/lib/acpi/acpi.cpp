/**
 * @file acpi.cpp
 * @brief ACPI table discovery and dispatch.
 *
 * ACPI (Advanced Configuration and Power Interface) provides firmware tables
 * that describe hardware configuration. The bootloader gives us the RSDP
 * address, and we follow the chain to find specific tables like the MADT.
 *
 * RSDP (ACPI 1.0) uses 32-bit pointers to the RSDT. XSDP (ACPI 2.0+) extends
 * it with 64-bit pointers to the XSDT. We use XSDP/XSDT since we're 64-bit.
 *
 * Table chain:
 *
 *   XSDP (Extended Root System Description Pointer)
 *     │
 *     └──▶ XSDT (Extended System Description Table)
 *            │
 *            ├──▶ MADT (APIC info)      → madt::parse()
 *            ├──▶ FADT (power mgmt)     → [not implemented]
 *            ├──▶ HPET (timers)         → [not implemented]
 *            └──▶ ...
 *
 * This file handles XSDP/XSDT parsing and dispatches to table-specific
 * parsers (e.g., madt.cpp) based on the 4-byte signature.
 */

#include "arch/x86_64/memory/vmm.hpp"
#include <arch.hpp>
#include <acpi/acpi.hpp>
#include <acpi/madt.hpp>
#include <fmt/fmt.hpp>
#include <kpanic/kpanic.hpp>
#include <log/log.hpp>

#include <cstdint>

namespace acpi {
    /**
     * @brief Computes checksum over a range of bytes.
     *
     * ACPI checksums are valid when the sum of all bytes equals 0 (mod 256).
     *
     * @param data Pointer to the data.
     * @param length Number of bytes to sum.
     * @return The checksum (0 if valid).
     */
    static std::uint8_t compute_checksum(const void* data, std::size_t length) {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(data);
        std::uint8_t sum = 0;

        for (std::size_t i = 0; i < length; i++) {
            sum += bytes[i];
        }

        return sum;
    }

    /**
     * @brief Validates the XSDP checksums.
     *
     * Validates both the legacy RSDP checksum (first 20 bytes) and the
     * extended checksum (all 36 bytes) for ACPI 2.0+.
     *
     * @param xsdp Pointer to the XSDP structure.
     */
    static void validate_xsdp(const XSDP* xsdp) {
        constexpr std::size_t RSDP_LEGACY_SIZE = 20;

        std::uint8_t legacy_sum = compute_checksum(xsdp, RSDP_LEGACY_SIZE);

        if (legacy_sum != 0) {
            kpanic("XSDP legacy checksum invalid: ", legacy_sum);
        }

        log::success("XSDP legacy checksum valid");

        if (xsdp->revision >= 2) {
            std::uint8_t extended_sum = compute_checksum(xsdp, xsdp->length);

            if (extended_sum != 0) {
                kpanic("XSDP extended checksum invalid: ", extended_sum);
            }

            log::success("XSDP extended checksum valid");
        }
    }

    /**
     * @brief Validates an ACPI table header checksum.
     *
     * The checksum covers the entire table (header.length bytes).
     *
     * @param header Pointer to the ACPI table header.
     */
    static void validate_acpi_header(const ACPIHeader* header) {
        kstring sig{header->signature, 4};
        std::uint8_t sum = compute_checksum(header, header->length);

        if (sum != 0) {
            kpanic("ACPI table checksum invalid: sig=", sig, " sum=", sum);
        }

        log::success("ACPI table checksum valid: ", sig);
    }

    static void log_xsdp(const XSDP* xsdp) {
        kstring signature{xsdp->signature, 8};
        kstring oem_id{xsdp->oem_id, 6};

        log::info("XSDP:");
        log::info("  signature    = \"", signature, "\"");
        log::info("  checksum     = ", xsdp->checksum);
        log::info("  oem_id       = ", oem_id);
        log::info("  revision     = ", xsdp->revision);
        log::info("  rsdt_addr    = ", fmt::hex{xsdp->rsdt_addr});
        log::info("  length       = ", xsdp->length);
        log::info("  xsdt_addr    = ", fmt::hex{xsdp->xsdt_addr});
        log::info("  ext_checksum = ", xsdp->extended_checksum);
        log::info("  reserved     = [", xsdp->reserved[0], ", ", xsdp->reserved[1], ", ", xsdp->reserved[2], "]");
    }

    static void log_acpi_header(const ACPIHeader* header) {
        kstring signature{header->signature, 4};
        kstring oem_id{header->oem_id, 6};
        kstring oem_table_id{header->oem_table_id, 8};

        log::info("ACPIHeader:");
        log::info("  signature        = \"", signature, "\"");
        log::info("  length           = ", header->length);
        log::info("  revision         = ", header->revision);
        log::info("  checksum         = ", header->checksum);
        log::info("  oem_id           = ", oem_id);
        log::info("  oem_table_id     = ", oem_table_id);
        log::info("  oem_revision     = ", header->oem_revision);
        log::info("  creator_id       = ", fmt::hex{header->creator_id});
        log::info("  creator_revision = ", header->creator_revision);
    }

    static void log_xsdt(const XSDT* xsdt) {
        log_acpi_header(&xsdt->header);

        int entries = (xsdt->header.length - sizeof(ACPIHeader)) / sizeof(std::uint64_t);
        log::info("XSDT entries = ", entries);
    }

    /**
     * @brief Retrieves an ACPI table header from its physical address.
     *
     * The XSDT contains physical addresses of other ACPI tables. This function
     * maps the given physical address into the HHDM address space so it can be accessed.
     *
     * @param phys_addr Physical address of the ACPI table.
     * @return Pointer to the ACPIHeader mapped in virtual memory.
     */
    static ACPIHeader* get_acpi_header(std::uint64_t phys_addr) {
        std::uintptr_t header_phys = static_cast<std::uintptr_t>(phys_addr);
        std::uintptr_t header_virt = arch::vmm::map_hddm_page(header_phys, arch::vmm::PAGE_WRITE | arch::vmm::PAGE_CACHE_DISABLE);

        return reinterpret_cast<ACPIHeader*>(header_virt);
    }

    /**
     * @brief Parses the XSDT and dispatches each table to its handler.
     *
     * Iterates through all ACPI table entries in the XSDT, maps each one into
     * virtual memory, and dispatches to the appropriate parser based on the
     * table's signature (e.g., MADT -> madt::parse).
     *
     * @param xsdt Pointer to the XSDT mapped in virtual memory.
     */
    static void parse_xsdt(const XSDT* xsdt) {
        log_xsdt(xsdt);
        validate_acpi_header(&xsdt->header);

        int entries = (xsdt->header.length - sizeof(ACPIHeader)) / sizeof(std::uint64_t);

        for (int i = 0; i < entries; i++) {
            ACPIHeader* header = get_acpi_header(xsdt->other_headers[i]);
            kstring sig{header->signature, 4};

            log_acpi_header(header);
            validate_acpi_header(header);

            if (sig == SIG_MADT) {
                madt::parse(header);
            } else {
                log::info("Skipping unhandled ACPI table: ", sig);
            }
        }
    }

    /**
     * @brief Retrieves the XSDT from the RSDP address.
     *
     * The XSDT address stored in the XSDP is a physical address. This function
     * maps it into the HHDM address space so it can be accessed.
     *
     * @param rsdp_addr Pointer to the RSDP/XSDP structure.
     * @return Pointer to the XSDT mapped in virtual memory.
     */
    static XSDT* get_xsdt(void* rsdp_addr) {
        auto* xsdp = reinterpret_cast<XSDP*>(rsdp_addr);
        log_xsdp(xsdp);
        validate_xsdp(xsdp);

        std::uintptr_t xsdt_phys = static_cast<std::uintptr_t>(xsdp->xsdt_addr);
        std::uintptr_t xsdt_virt = arch::vmm::map_hddm_page(xsdt_phys, arch::vmm::PAGE_WRITE | arch::vmm::PAGE_CACHE_DISABLE);

        return reinterpret_cast<XSDT*>(xsdt_virt);
    }

    void init(void* rsdp_addr) {
        log::init_start("ACPI");

        XSDT* xsdt = get_xsdt(rsdp_addr);
        parse_xsdt(xsdt);

        log::init_end("ACPI");
    }
}
