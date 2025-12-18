#include "kernel/panic/panic.hpp"

#include "kernel/log/log.hpp"
#include "arch/i686/cpu/cpu.hpp"

namespace kernel {
    // TODO: kernel library code should not have arch specific dependencies
    // like i686::cpu::cli(), this should be abstracted away using something like
    // a preprocessor macro to check the current cpu architecture. For now this
    // will be left as is to reduce complexity.
    void panic(const char* str) {
        i686::cpu::cli();

        log::error("*** KERNEL PANIC ***");
        log::error(str);
        log::error("System will now halt.");

        while (true) {
            i686::cpu::hlt();
        }
    }

    void panicf(const char* format, ...) {
        i686::cpu::cli();

        /*
        std::va_list args;
        va_start(args, format);

        log::error("*** KERNEL PANIC ***");

        kernel::kvprintf(format, args);
        kernel::kprintf("\n");
        
        log::error("System will now halt.");

        va_end(args);
        */

        while (true) {
            i686::cpu::hlt();
        }
    }
}
