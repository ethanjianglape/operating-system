#include "algo/algo.hpp"
#include "containers/kvector.hpp"
#include <console/ansi.hpp>
#include <console/console.hpp>
#include <cstdint>
#include <log/log.hpp>
#include <fmt/fmt.hpp>

namespace console::ansi {
    bool is_csi(kstring::const_iterator iter) {
        return *iter == ANSI_ESCAPE && *(iter + 1) == '[';
    }

    bool is_cs_code(char c) {
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

    kstring get_default_arg(char c) {
        switch (c) {
        case CURSOR_UP:
        case CURSOR_DOWN:
        case CURSOR_FORWARD:
        case CURSOR_BACK:
        case CURSOR_NEXT_LINE:
        case CURSOR_PREV_LINE:
        case CURSOR_HORI_ABSO:
        case CURSOR_POSITION:
        case SCROLL_UP:
        case SCROLL_DOWN:
            return "1";
        case ERASE_IN_DISPLAY:
        case ERASE_IN_LINE:
        case SELECT_GRAPHIC_RENDITION:
            return "0";
        default:
            return "";
        }
    }

    void cursor_forward(const kvector<kstring>& args) {
        const kstring& arg = args.front();
        std::uintmax_t val = fmt::parse_uint(arg);

        console::move_cursor(val, 0);
    }

    void cursor_back(const kvector<kstring>& args) {
        const kstring& arg = args.front();
        std::uintmax_t val = fmt::parse_uint(arg);

        console::move_cursor(-val, 0);
    }

    void erase_in_line(const kvector<kstring>& args) {
        const kstring& arg = args.front();
        std::uintmax_t val = fmt::parse_uint(arg);

        if (val == 0) {
            console::erase_in_line(console::get_cursor_x(), console::get_screen_cols());
        } else if (val == 1) {
            console::erase_in_line(0, console::get_cursor_x());
        } else if (val == 2) {
            console::erase_in_line(0, console::get_screen_cols());
        } else {
            log::warn("Invalid Erase In Line arg: ", val);
        }
    }

    void cursor_next_line(const kvector<kstring>& args) {
        const kstring& arg = args.front();
        std::uintmax_t val = fmt::parse_uint(arg);

        while (val > 0) {
            console::newline();
            val--;
        }
    }

    std::size_t parse_csi(kstring::const_iterator begin, kstring::const_iterator str_end) {
        auto iter = begin;

        while (iter != str_end) {
            if (is_cs_code(*iter)) {
                break;
            }

            ++iter;
        }

        if (iter == str_end) {
            log::warn("Unsupported ANSI escape sequence 123");
            return 0;
        }

        char code = *iter;

        kstring default_arg = get_default_arg(code);
        kvector<kstring> args = algo::tokenize(begin + 2, iter, ';');

        log::debug("num args: ", args.size());

        if (args.empty()) {
            args.push_back(default_arg);
        } else {
            for (auto& arg : args) {
                log::debug("arg: '", arg.size(), "'");
                
                if (arg.empty()) {
                    log::debug("empty arg");
                    arg = default_arg;
                }
            }
        }

        log::debug("ANSI code: ", code);
        log::debug("Default arg: ", default_arg);


        for (const auto& arg : args) {
            log::debug("Arg: ", arg);
        }

        auto distance = (iter - begin) + 1;

        log::debug("str len = ", distance);

        switch (code) {
        case CURSOR_FORWARD:
            cursor_forward(args);
            break;
        case CURSOR_BACK:
            cursor_back(args);
            break;
        case ERASE_IN_LINE:
            erase_in_line(args);
            break;
        case CURSOR_NEXT_LINE:
            cursor_next_line(args);
            break;
        }

        return distance;
    }
    
    std::size_t parse_ansi_escape(kstring::const_iterator iter, kstring::const_iterator str_end) {
        if (is_csi(iter)) {
            return parse_csi(iter, str_end);
        }
        
        log::warn("Unsupported ANSI escape sequence");
        
        return 1; // return 1 so this character isn't parsed again
    }
}
