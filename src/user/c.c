#define SYS_WRITE 1
#define SYS_SLEEP 35
#define SYS_EXIT  60

typedef unsigned long size_t;
typedef long ssize_t;

static inline ssize_t syscall1(int num, unsigned long arg1) {
    ssize_t ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(arg1)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline ssize_t syscall3(int num, unsigned long arg1, unsigned long arg2, unsigned long arg3) {
    ssize_t ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

ssize_t write(int fd, const char* buf, size_t count) {
    return syscall3(SYS_WRITE, fd, (unsigned long)buf, count);
}

ssize_t sleep_ms(unsigned long ms) {
    return syscall1(SYS_SLEEP, ms);
}

void exit(int status) {
    syscall1(SYS_EXIT, status);
}

void _start(void) {
    for (int i = 0; i < 5; i++) {
        sleep_ms(1000);
        write(1, "C", 1);
    }
    exit(0);
}
