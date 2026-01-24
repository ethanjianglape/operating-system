#define SYS_READ  0
#define SYS_WRITE 1
#define SYS_SLEEP 35

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

ssize_t read(int fd, char* buf, size_t count) {
    return syscall3(SYS_READ, fd, (unsigned long)buf, count);
}

ssize_t sleep_ms(unsigned long ms) {
    return syscall1(SYS_SLEEP, ms);
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

void print(const char* s) {
    write(1, s, strlen(s));
}

void _start(void) {
    print("Hello from userspace!\n");

    while (1) {
        print("/ >");
        
        char buf[128];
        ssize_t n = read(0, buf, sizeof(buf) - 1);

        if (n > 0) {
            buf[n] = '\0';
            print("You typed: ");
            write(1, buf, n);
            print("\n");
        }

        print("sleeping...\n");
        sleep_ms(5000);
    }

    while (1) {}
}
