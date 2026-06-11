#pragma once

#include "fmt/fmt.hpp"
#include <containers/klist.hpp>
#include <containers/kstring.hpp>
#include <containers/kvector.hpp>

namespace algo {
// Splits a string into components around a delim.
// Removes empty values.
// ex: split("a::b:c:::d", ':') -> ["a", "b", "c", "d"]
kvector<kstring> split(kstring::const_iterator begin,
    kstring::const_iterator                    end,
    char                                       delim = ' ');

// Splits a string into components around a delim.
// Keeps empty values.
// ex: split("a::b:c:::d", ':') -> ["a", "", "b", "c", "", "", "d"]
kvector<kstring> tokenize(kstring::const_iterator begin,
    kstring::const_iterator                       end,
    char                                          delim = ' ');

kvector<kstring> split(const kstring& str, char delim = ' ');

template <typename Iter>
kstring join(Iter begin,
    Iter          end,
    char          delim = ' ')
{
    kstring result;

    while (begin != end) {
        result += *begin;
        result += delim;
        ++begin;
    }

    if (!result.empty()) {
        result.pop_back();
    }

    return result;
}

template <typename T>
kstring join(const kvector<T>& v, char delim = ' ')
{
    return join(v.begin(), v.end(), delim);
}

template <typename T>
kstring join(const klist<T>& list, char delim = ' ')
{
    kstring result;

    for (std::size_t i = 0; i < list.size(); i++) {
        const T& item = list[i];

        result += fmt::to_string(item);
        result.push_back(delim);
    }

    if (!result.empty()) {
        result.pop_back();
    }

    return result;
}
}
