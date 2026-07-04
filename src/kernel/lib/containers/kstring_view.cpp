#include <arch.hpp>
#include <containers/kstring_view.hpp>
#include <kassert/kassert.hpp>

kstring_view kstring_view::from_userspace(const char* chars)
{
    kassert(arch::vmm::is_user_addr(chars));
    arch::cpu::stac();
    kstring_view str{chars};
    arch::cpu::clac();
    return str;
}
