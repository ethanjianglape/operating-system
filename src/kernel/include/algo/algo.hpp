#pragma once

#include <containers/kstring.hpp>
#include <containers/kvector.hpp>

namespace algo {
    kvector<kstring> split(kstring::const_iterator begin, kstring::const_iterator end, char delim = ' ');
    kvector<kstring> split(const kstring& str, char delim = ' ');
    kstring join(const kvector<kstring>& parts, char delim = ' ');
}
