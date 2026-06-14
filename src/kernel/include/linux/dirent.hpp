#pragma once

namespace linux {
struct [[gnu::packed]] linux_dirent64 {
    unsigned long d_ino;
    unsigned long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};
}
