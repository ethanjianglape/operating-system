// Minimal userspace program - does nothing, just exits
// Will need syscall support to actually do anything useful

void _start(void) {
    // Loop forever for now - no way to exit without syscalls
    while (1) {
        // TODO: syscall to exit
    }
}
