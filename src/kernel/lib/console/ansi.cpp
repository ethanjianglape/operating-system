#include "algo/algo.hpp"
#include "containers/kvector.hpp"
#include <console/ansi.hpp>
#include <console/console.hpp>
#include <log/log.hpp>
#include <fmt/fmt.hpp>

namespace console::ansi {
    bool is_csi(kstring::const_iterator iter) {
        return *iter == ANSI_ESCAPE && *(iter + 1) == '[';
    }

    bool is_cs(char c) {
        switch (c) {
        case CURSOR_UP:
        case CURSOR_DOWN:
        case CURSOR_FORWARD:
        case CURSOR_BACK:
        case CURSOR_NEXT_LINE:
        case CURSOR_PREV_LINE:
        case CURSOR_HORI_ABSO:
        case CURSOR_POSITION:
        case ERASE_IN_DISPLAY:
        case ERASE_IN_LINE:
        case SCROLL_UP:
        case SCROLL_DOWN:
        case SELECT_GRAPHIC_RENDITION:
            return true;
        default:
            return false;
        }
    }

    kvector<kstring> parse_args(kstring::const_iterator begin, kstring::const_iterator end) {

    }

    std::size_t parse_csi(kstring::const_iterator begin, kstring::const_iterator end) {
        auto iter = begin;

        while (iter != end) {
            if (is_cs(*iter)) {
                break;
            }
        }

        if (iter == end) {
            log::warn("Unsupported ANSI escape sequence");
            return 0;
        }

        kvector<kstring> args = algo::split(begin, iter, ';');
        
        return 0;
    }
    
    std::size_t parse_ansi_escape(kstring::const_iterator iter, kstring::const_iterator str_end) {
        if (is_csi(iter)) {
            return parse_csi(iter + 2, str_end);
        }
        
        log::warn("Unsupported ANSI escape sequence");
        return 0;
    }
}
