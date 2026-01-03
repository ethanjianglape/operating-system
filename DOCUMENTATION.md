# Documentation Standards

This document defines the documentation conventions used throughout the codebase.

## Comment Hierarchy

We use a three-tier comment system:

### 1. Doxygen (`/** ... */`) — API Documentation

Use for **file headers** and **function definitions**. These document the interface and contract.

**File header example:**
```cpp
/**
 * @file apic.cpp
 * @brief Advanced Programmable Interrupt Controller (APIC) driver.
 *
 * Detailed description of what this module does...
 */
```

**Function example:**
```cpp
/**
 * @brief Sets the Local APIC base address by mapping the physical address.
 * @param lapic_phys_addr Physical address of the LAPIC (from ACPI MADT).
 */
void set_lapic_addr(std::uintptr_t lapic_phys_addr);
```

**Common Doxygen tags:**
- `@file` — File name
- `@brief` — Short description (first line)
- `@param` — Parameter description
- `@return` — Return value description
- `@pre` — Preconditions
- `@post` — Postconditions
- `@note` — Additional information
- `@warning` — Important warnings
- `@see` — Cross-references

### 2. Section Banners (`// ===`) — Code Organization

Use for **grouping related functions** within a file. These are visual separators for humans navigating the source, not parsed by documentation tools.

```cpp
// =========================================================================
// LAPIC Register Access
// =========================================================================
//
// Explanation of this section...
```

### 3. Inline Comments (`//`) — Implementation Details

Use for **explaining code within function bodies**. These describe *how* and *why* the implementation works.

```cpp
// Step 1: Enable the APIC globally via the MSR
enable_apic();

// The divider slows down the timer. With DIV_BY_16, the timer counts
// at (bus clock / 16). This gives us more precision for slower tick rates.
lapic_write(LAPIC_TIMER_DIVIDE, TIMER_DIV_BY_16);
```

## Documentation Location

We document in `.cpp` files rather than `.hpp` headers. This deviates from the
typical C++ convention of documenting in headers, but is intentional:

- **This is a kernel, not a library** — Headers aren't exposed to external
  consumers. Anyone reading this code has access to both files.

- **Learning-friendly** — Keeping documentation next to the implementation
  means readers don't need to jump between files to understand what's happening.

- **Hardware details belong with code** — Register layouts, magic numbers, and
  CPU behavior explanations are most useful right where they're used.

- **Many functions are internal** — Not every function is declared in headers.
  Internal helpers like `make_gdt_entry()` or `lapic_read()` only exist in the
  `.cpp` file, so their documentation naturally lives there too.

## Guidelines

1. **Don't state the obvious** — If the function name already says it all, skip
   the Doxygen comment. `User make_user()` doesn't need `@brief Makes a user`.
   Only add documentation when it clarifies something non-obvious.

2. **Don't over-document** — If the code is self-explanatory, don't add a
   comment. Prefer clear naming over comments.

3. **Hardware-specific code gets more comments** — Low-level hardware interaction (APIC, GDT, etc.) 
   benefits from extensive comments explaining register layouts and magic numbers, since these can't be intuited.

4. **Keep comments updated** — Wrong comments are worse than no comments.
