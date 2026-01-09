#pragma once

#include <containers/kstring.hpp>
#include <containers/kvector.hpp>

namespace algo {
    // Splits a string into components around a delim.
    // Removes empty values.
    // ex: split("a::b:c:::d", ':') -> ["a", "b", "c", "d"]
    kvector<kstring> split(kstring::const_iterator begin,
                           kstring::const_iterator end,
                           char delim = ' ');

    // Splits a string into components around a delim.
    // Keeps empty values.
    // ex: split("a::b:c:::d", ':') -> ["a", "", "b", "c", "", "", "d"]
    kvector<kstring> tokenize(kstring::const_iterator begin,
                              kstring::const_iterator end,
                              char delim = ' ');
    
    kvector<kstring> split(const kstring& str, char delim = ' ');
    kstring join(const kvector<kstring>& parts, char delim = ' ');
}
