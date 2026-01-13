#pragma once

#include <containers/kstring.hpp>

namespace console::ansi {
    constexpr char ANSI_ESCAPE = '\033';
    
    constexpr char CURSOR_UP        = 'A';
    constexpr char CURSOR_DOWN      = 'B';
    constexpr char CURSOR_FORWARD   = 'C';
    constexpr char CURSOR_BACK      = 'D';
    constexpr char CURSOR_NEXT_LINE = 'E';
    constexpr char CURSOR_PREV_LINE = 'F';
    constexpr char CURSOR_HORI_ABSO = 'G';
    constexpr char CURSOR_POSITION  = 'H';
    
    constexpr char ERASE_IN_DISPLAY = 'J';
    constexpr char ERASE_IN_LINE    = 'K';
    
    constexpr char SCROLL_UP   = 'S';
    constexpr char SCROLL_DOWN = 'T';
    
    constexpr char SELECT_GRAPHIC_RENDITION = 'm';

    std::size_t parse_ansi_escape(kstring::const_iterator iter, kstring::const_iterator str_end);
}
