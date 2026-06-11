# Kernel Code Conventions

## Namespaces

**No `kernel::` prefix** - everything is already in the kernel.

Use flat subsystem namespaces:
```cpp
namespace pmm { }      // physical memory manager
namespace log { }      // logging
namespace tty { }      // terminal
namespace console { }  // framebuffer console
```

Architecture-specific code uses `x64::` (or other arch):
```cpp
namespace x64::vmm { }
namespace x64::drivers::apic { }
```

**Global utilities use k-prefix** (like Linux's `printk`):
```cpp
kstring, kvector, kprint(), kprintln()
```

**Implementation details** use prefixed detail namespaces:
```cpp
namespace kprint_detail { }  // good - clear ownership
namespace detail { }         // bad - ambiguous at global scope
```

## Architecture Abstraction

`lib/` code must use the `arch::` abstraction, never `x64::` directly:
```cpp
// in lib/panic/panic.cpp
#include <arch.hpp>
arch::cpu::cli();     // good
x64::cpu::cli();   // bad - breaks portability
```

Only `include/arch.hpp` bridges architectures:
```cpp
namespace arch {
    namespace vmm = ::x64::vmm;
    namespace cpu = ::x64::cpu;
    namespace drivers = ::x64::drivers;
}
```

Arch code (`arch/x64/`) can use its own namespace directly.

## File Structure

```
kernel/
├── include/          # headers (flat structure)
│   ├── arch.hpp      # arch abstraction
│   ├── log/log.hpp
│   ├── kprint/kprint.hpp
│   └── ...
├── lib/              # implementations (mirrors include/)
│   ├── log/log.cpp
│   └── ...
└── arch/x64/      # self-contained, headers next to source
    ├── vmm/
    │   ├── vmm.hpp
    │   └── vmm.cpp
    └── ...
```

## Includes

```cpp
#include <log/log.hpp>    // non-local: angle brackets
#include "local.hpp"      // same directory only: quotes
```

