#include <algo/algo.hpp>

namespace algo {
    kvector<kstring> split(const kstring& str, char delim) {
        kvector<kstring> result{};
        kstring part = "";
        
        if (str.empty()) {
            return result;
        }

        for (char c : str) {
            if (c == delim) {
                if (!part.empty()) {
                    result.push_back(part);
                    part = "";
                }
            } else {
                part += c;
            }
        }

        if (!part.empty()) {
            result.push_back(part);
        }
        
        return result;
    }
}
