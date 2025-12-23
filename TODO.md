# Future Improvements

## Index

- [ ] [Logging and Console Separation](#logging-and-console-separation)
- [ ] [IOAPIC GSI Mapping](#ioapic-gsi-mapping)
- [ ] [PS/2 Controller Initialization](#ps2-controller-initialization)

---

## Logging and Console Separation

**Status:** Not started

**Current state:** kprintf outputs to both serial and framebuffer console, mixing kernel debug output with user-facing display.

**Target architecture:**
```
Debug path:   kprintf() → serial only (unlimited, for kernel devs)
User path:    TTY → console → framebuffer (clean, user-facing)
```

**Implementation:**
- kprintf/kernel::log writes directly to serial output
- kernel::console becomes purely for TTY/user interaction
- TTY handles keyboard input, input buffer, line editing
- Console handles character rendering, cursor, scrolling
- Future: Add log levels to optionally route important messages to console

**Benefits:**
- No debug spew corrupting user's input line
- Unlimited serial logging without framebuffer scroll concerns
- Easier debugging via QEMU serial capture
- Clean separation of concerns

---

## IOAPIC GSI Mapping

**Status:** Not started

**Current state:** Hardcoded assumptions for interrupt routing:
- GSI base = 0
- No IRQ overrides (IRQ N = GSI N = IOAPIC pin N)
- ISA defaults (edge triggered, active high)

**Correct implementation:**
- Parse and store MADT Interrupt Source Overrides
- Lookup function to resolve IRQ → GSI with override check
- Find correct IOAPIC by GSI range (for multi-IOAPIC systems)
- Apply polarity/trigger flags from overrides when programming redirection entries

---

## PS/2 Controller Initialization

**Status:** Not started

**Current state:** Assumes PS/2 controller and keyboard are present and working. Directly reads scancodes from port 0x60.

**Correct implementation:**
- Disable both PS/2 ports during initialization
- Flush the output buffer (read port 0x60 until status bit 0 is clear)
- Read and modify controller configuration byte
- Perform controller self-test (command 0xAA, expect 0x55)
- Test first PS/2 port (command 0xAB, expect 0x00)
- Test second PS/2 port if present (command 0xA9)
- Enable ports and enable interrupts in config byte
- Reset keyboard device (send 0xFF, expect 0xFA ACK)
- Set scancode set if needed (default is usually fine)
- Handle initialization failures gracefully
