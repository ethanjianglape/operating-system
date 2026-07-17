#include <arch.hpp>
#include <containers/kstring.hpp>
#include <kassert/kassert.hpp>

#include <cstdint>

kstring kstring::from_userspace(const char* chars)
{
    kassert(arch::vmm::is_user_addr(chars));

    const std::uint64_t rflags = arch::cpu::read_rflags();
    arch::cpu::stac();
    kstring str{chars};
    arch::cpu::write_rflags(rflags);

    return str;
}
