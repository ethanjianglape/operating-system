/**
 * Userspace shell for MyOS
 *
 * A simple interactive shell demonstrating syscall usage.
 */

// ============================================================================
// Types
// ============================================================================

typedef unsigned long size_t;
typedef long ssize_t;
typedef int pid_t;

// ============================================================================
// Syscall numbers (Linux x86_64 compatible where possible)
// ============================================================================

#define SYS_READ     0
#define SYS_WRITE    1
#define SYS_OPEN     2
#define SYS_CLOSE    3
#define SYS_STAT     4
#define SYS_FSTAT    5
#define SYS_LSEEK    8
#define SYS_MMAP     9
#define SYS_GETPID   39
#define SYS_EXIT     60
#define SYS_GETCWD   79
#define SYS_CHDIR    80

// Non-standard (MyOS specific)
#define SYS_SLEEP_MS 35

// ============================================================================
// mmap flags
// ============================================================================

#define PROT_READ    0x1
#define PROT_WRITE   0x2
#define PROT_EXEC    0x4
#define PROT_NONE    0x0

#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20

// ============================================================================
// File types and flags
// ============================================================================

#define O_RDONLY 0x01

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// File types (matching kernel fs::FileType)
#define FT_NONE    0
#define FT_REGULAR 1
#define FT_DIR     2
#define FT_CHARDEV 3

struct stat {
    unsigned char type;
    size_t size;
};

// ============================================================================
// Syscall wrappers
// ============================================================================

static inline ssize_t syscall0(int num) {
    ssize_t ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(num)
        : "rcx", "r11", "memory"
    );
    return ret;
}

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

static inline ssize_t syscall2(int num, unsigned long arg1, unsigned long arg2) {
    ssize_t ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(arg1), "S"(arg2)
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

static inline ssize_t syscall6(int num, unsigned long arg1, unsigned long arg2,
                               unsigned long arg3, unsigned long arg4,
                               unsigned long arg5, unsigned long arg6) {
    ssize_t ret;
    register unsigned long r10 asm("r10") = arg4;
    register unsigned long r8  asm("r8")  = arg5;
    register unsigned long r9  asm("r9")  = arg6;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory"
    );
    return ret;
}

// ============================================================================
// Libc-style syscall wrappers
// ============================================================================

ssize_t read(int fd, void* buf, size_t count) {
    return syscall3(SYS_READ, fd, (unsigned long)buf, count);
}

ssize_t write(int fd, const void* buf, size_t count) {
    return syscall3(SYS_WRITE, fd, (unsigned long)buf, count);
}

int open(const char* path, int flags) {
    return syscall2(SYS_OPEN, (unsigned long)path, flags);
}

int close(int fd) {
    return syscall1(SYS_CLOSE, fd);
}

int stat(const char* path, struct stat* st) {
    return syscall2(SYS_STAT, (unsigned long)path, (unsigned long)st);
}

int fstat(int fd, struct stat* st) {
    return syscall2(SYS_FSTAT, fd, (unsigned long)st);
}

ssize_t lseek(int fd, size_t offset, int whence) {
    return syscall3(SYS_LSEEK, fd, offset, whence);
}

pid_t getpid(void) {
    return syscall0(SYS_GETPID);
}

void exit(int status) {
    syscall1(SYS_EXIT, status);
    __builtin_unreachable();
}

int getcwd(char* buf, size_t size) {
    return syscall2(SYS_GETCWD, (unsigned long)buf, size);
}

int chdir(const char* path) {
    size_t len = 0;
    while (path[len]) len++;
    return syscall2(SYS_CHDIR, (unsigned long)path, len);
}

// MyOS specific
ssize_t sleep_ms(unsigned long ms) {
    return syscall1(SYS_SLEEP_MS, ms);
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, size_t offset) {
    return (void*)syscall6(SYS_MMAP, (unsigned long)addr, length, prot, flags, fd, offset);
}

// ============================================================================
// String utilities
// ============================================================================

size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strncmp(const char* a, const char* b, size_t n) {
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)a - *(unsigned char*)b;
}

char* strcpy(char* dst, const char* src) {
    char* ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

// ============================================================================
// I/O helpers
// ============================================================================

void print(const char* s) {
    write(1, s, strlen(s));
}

void println(const char* s) {
    print(s);
    write(1, "\n", 1);
}

void print_int(int n) {
    if (n < 0) {
        write(1, "-", 1);
        n = -n;
    }
    if (n == 0) {
        write(1, "0", 1);
        return;
    }

    char buf[20];
    int i = 0;
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i > 0) {
        write(1, &buf[--i], 1);
    }
}

void print_hex(unsigned long n) {
    print("0x");
    if (n == 0) {
        write(1, "0", 1);
        return;
    }

    char buf[20];
    int i = 0;
    while (n > 0) {
        int digit = n & 0xF;
        buf[i++] = digit < 10 ? '0' + digit : 'a' + (digit - 10);
        n >>= 4;
    }
    while (i > 0) {
        write(1, &buf[--i], 1);
    }
}

// ============================================================================
// Shell commands
// ============================================================================

void cmd_help(void) {
    println("Available commands:");
    println("  help     - Show this help");
    println("  pwd      - Print working directory");
    println("  cd PATH  - Change directory");
    println("  pid      - Show process ID");
    println("  mmap     - Test mmap syscall");
    println("  exit     - Exit shell");
}

void cmd_pwd(void) {
    char buf[256];
    if (getcwd(buf, sizeof(buf)) == 0) {
        println(buf);
    } else {
        println("getcwd failed");
    }
}

void cmd_cd(const char* path) {
    if (chdir(path) != 0) {
        print("cd: no such directory: ");
        println(path);
    }
}

void cmd_pid(void) {
    print("PID: ");
    print_int(getpid());
    print("\n");
}

void cmd_stat(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        print("stat: cannot stat '");
        print(path);
        println("'");
        return;
    }

    print(path);
    print(": ");

    switch (st.type) {
    case FT_REGULAR:
        print("regular file, ");
        break;
    case FT_DIR:
        print("directory, ");
        break;
    case FT_CHARDEV:
        print("char device, ");
        break;
    default:
        print("unknown type, ");
        break;
    }

    print("size=");
    print_int(st.size);
    print("\n");
}

void cmd_mmap(void) {
    println("Testing mmap...");

    // Allocate one page (4096 bytes) of anonymous memory
    size_t size = 4096;
    void* ptr = mmap((void*)0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    print("  mmap returned: ");
    print_hex((unsigned long)ptr);
    print("\n");

    // Check for failure (MAP_FAILED is typically -1)
    if (ptr == (void*)-1 || ptr == (void*)0) {
        println("  FAILED: mmap returned invalid pointer");
        return;
    }

    // Write a test pattern
    unsigned char* mem = (unsigned char*)ptr;
    for (size_t i = 0; i < size; i++) {
        mem[i] = (unsigned char)(i & 0xFF);
    }
    println("  Wrote test pattern to memory");

    // Read back and verify
    int errors = 0;
    for (size_t i = 0; i < size; i++) {
        if (mem[i] != (unsigned char)(i & 0xFF)) {
            errors++;
        }
    }

    if (errors == 0) {
        println("  SUCCESS: All bytes verified correctly");
    } else {
        print("  FAILED: ");
        print_int(errors);
        println(" byte mismatches");
    }
}

// ============================================================================
// Command parser
// ============================================================================

void process_command(char* cmd) {
    // Skip leading whitespace
    while (*cmd == ' ') cmd++;

    // Empty command
    if (*cmd == '\0') return;

    // Parse command and argument
    char* arg = cmd;
    while (*arg && *arg != ' ') arg++;
    if (*arg == ' ') {
        *arg++ = '\0';
        while (*arg == ' ') arg++;
    }

    // Dispatch
    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "pwd") == 0) {
        cmd_pwd();
    } else if (strcmp(cmd, "cd") == 0) {
        if (*arg) {
            cmd_cd(arg);
        } else {
            cmd_cd("/");
        }
    } else if (strcmp(cmd, "pid") == 0) {
        cmd_pid();
    } else if (strcmp(cmd, "mmap") == 0) {
        cmd_mmap();
    } else if (strcmp(cmd, "stat") == 0) {
        if (*arg) {
            cmd_stat(arg);
        } else {
            println("usage: stat PATH");
        }
    } else if (strcmp(cmd, "exit") == 0) {
        exit(0);
    } else {
        print("unknown command: ");
        println(cmd);
    }
}

// ============================================================================
// Entry point
// ============================================================================

void _start(void) {
    println("MyOS Shell");
    println("Type 'help' for available commands.\n");

    char buf[256];

    while (1) {
        // Print prompt with cwd
        char cwd[128];
        if (getcwd(cwd, sizeof(cwd)) == 0) {
            print(cwd);
        }
        print(" $ ");

        // Read input
        ssize_t n = read(0, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            process_command(buf);
        }
    }
}
