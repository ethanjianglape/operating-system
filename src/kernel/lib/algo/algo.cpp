#include <algo/algo.hpp>

namespace algo {
    kvector<kstring> split(kstring::const_iterator begin,
                           kstring::const_iterator end,
                           char delim) {
        kvector<kstring> result{};
        kstring part = "";

        while (begin != end) {
            char c = *begin;

            if (c == delim) {
                if (!part.empty()) {
                    result.push_back(part);
                    part = "";
                }
            } else {
                part += c;
            }

            ++begin;
        }

        if (!part.empty()) {
            result.push_back(part);
        }

        return result;
    }

    kvector<kstring> tokenize(kstring::const_iterator begin,
                              kstring::const_iterator end,
                              char delim) {
        kvector<kstring> result{};
        kstring part = "";
        char last = '\0';

        while (begin != end) {
            char c = *begin;
            last = c;

            if (c == delim) {
                result.push_back(part);
                part = "";
            } else {
                part += c;
            }

            ++begin;
        }

        if (!part.empty() || last == delim) {
            result.push_back(part);
        }

        return result;
    }
    
    kvector<kstring> split(const kstring& str, char delim) {
        return split(str.begin(), str.end(), delim);
    }

    kstring join(const kvector<kstring>& parts, char delim) {
        kstring result;

        for (const auto& part : parts) {
            result += part;
            result += delim;
        }

        if (!result.empty()) {
            result.pop_back();
        }

        return result;
    }
}
