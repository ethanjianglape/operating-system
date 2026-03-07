#pragma once

namespace syscall {
    long sys_set_tid_address(int* tidptr);
}
