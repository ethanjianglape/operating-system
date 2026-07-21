#include <acpi/acpi.hpp>
#include <acpi/hpet.hpp>
#include <fmt/fmt.hpp>
#include <kassert/kassert.hpp>
#include <log/log.hpp>

namespace acpi::hpet {

void parse(ACPIHeader* header)
{
    kassert_not_null(header);

    auto* hpet = reinterpret_cast<HPET*>(header);

    log::infof("HPET:");
    log::infof("  hardware_rev_id  = {}", hpet->hardware_rev_id);
    log::infof("  comparator_count = {}", hpet->comparator_count);
    log::infof("  counter_size     = {}", hpet->counter_size);
    log::infof("  hpet_number      = {}", hpet->hpet_number);
    log::infof("  minimum_tick     = {}", hpet->minimum_tick);
    log::infof("  page_protection  = {}", hpet->page_protection);
    log::infof("  address          = {}", fmt::hex{hpet->address.address});
}

}
