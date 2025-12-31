// Minimal userspace program - does nothing, just exits
// Will need syscall support to actually do anything useful

void write(int fd, const char* str, int size) {
    int syscall = 1; // write

    asm volatile("syscall;"
                 :
                 : "a"(1), "D"((unsigned long)fd), "S"(str), "d"(size)
                 : "rcx", "r11", "memory");
}

void _start(void) {
    // Loop forever for now - no way to exit without syscalls

    char* str = "\nhello from userspace!\n";
    char* len = str;

    while (*len++);

    write(1, str, len - str);
    
    while (1) {
        // TODO: syscall to exit
    }
}
