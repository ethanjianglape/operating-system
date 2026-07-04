#include <arch.hpp>
#include <containers/kstring.hpp>
#include <kassert/kassert.hpp>

kstring kstring::from_userspace(const char* chars)
{
    kassert(arch::vmm::is_user_addr(chars));
    arch::cpu::stac();
    kstring str{chars};
    arch::cpu::clac();
    return str;
}
