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

Use sparingly for **grouping related functions** within a file. These are visual separators for humans navigating the source, not parsed by documentation tools.

```cpp
// =========================================================================
// LAPIC Register Access
// =========================================================================
//
// Explanation of this section...
```

**When to use section banners:**
- Only when a file has multiple distinct groups of related functions
- Never for a single function — if there's only one function in the "section", it doesn't need a banner

**When NOT to use section banners:**
- The desire to add a section banner often indicates the code should be in its own file or namespace instead. Consider refactoring before reaching for a banner.

### 3. Inline Comments (`//`) — Implementation Details

Use for **explaining code within function bodies**. These describe *how* and *why* the implementation works.

```cpp
// Step 1: Enable the APIC globally via the MSR
enable_apic();

// The divider slows down the timer. With DIV_BY_16, the timer counts
// at (bus clock / 16). This gives us more precision for slower tick rates.
lapic_write(LAPIC_TIMER_DIVIDE, TIMER_DIV_BY_16);
```

**When NOT to use inline comments:**
- Don't comment self-explanatory function calls — `enable_apic()` doesn't need `// Enable the APIC`
- Don't repeat information already in a doxygen comment — if the function's `@brief` explains what a call does, don't echo it inline

## Assembly Files (`.s`)

Assembly files use semicolon-based comments instead of `#` or `//`:

```asm
;;; ============================================================================
;;; File/section headers use triple semicolons
;;; ============================================================================
;;;
;;; Extended explanations also use triple semicolons.
;;; These are for documentation blocks at the top of files or before functions.

.global my_function
my_function:
    ;; Inline comments explaining the next few instructions use double semicolons
    push %rbp
    mov %rsp, %rbp

    xor %eax, %eax      ; Single semicolon for end-of-line comments
    ret
```

**Summary:**
- `;;;` — File headers, section banners, multi-line documentation blocks
- `;;` — Inline comments explaining the following code
- `;` — End-of-line comments on the same line as an instruction

This convention renders better in some editors (notably Emacs) and visually
distinguishes comment "weight" similar to the C++ hierarchy.

## Documentation Location

We document in `.cpp` files rather than `.hpp` headers. This deviates from the
typical C++ convention of documenting in headers, but is intentional:

**The philosophy:** `.cpp` files are the primary learning resource. When someone
wants to understand how the GDT works, they should open `gdt.cpp` and find a
detailed explanation directly above the code that actually does it. A 100-line
comment in a header next to `void init_gdt();` doesn't help — you can't see what
the code is doing. But the same comment above the actual implementation lets you
read the explanation and the code together, going back and forth as needed.

**Why this works for a kernel:**

- **This is a kernel, not a library** — Headers aren't exposed to external
  consumers. Anyone reading this code has access to both files.

- **Implementation IS the interface** — In library code, you document headers
  because users only see headers. Here, readers see everything, and the
  implementation is what they're trying to understand.

- **Hardware details belong with code** — Register layouts, magic numbers, and
  CPU behavior explanations are most useful right where they're used.

**The rule:** If a `.cpp` file exists, put the `@file` header and detailed
documentation there. The `.hpp` should have minimal comments — just enough
to understand struct layouts or function signatures at a glance.

**Exceptions for header-only code:**
- Header-only files (no `.cpp`) put documentation in the header (e.g., `context.hpp`)
- Template functions and constants have no `.cpp` equivalent, so document them in the header
- Constants don't need doxygen — a simple `//` comment is sufficient

## Guidelines

1. **Don't state the obvious** — If the function name already says it all, skip
   the Doxygen comment. `User make_user()` doesn't need `@brief Makes a user`.
   Only add documentation when it clarifies something non-obvious.

2. **Don't over-document** — If the code is self-explanatory, don't add a
   comment. Prefer clear naming over comments.

3. **Hardware-specific code gets more comments** — Low-level hardware interaction (APIC, GDT, etc.) 
   benefits from extensive comments explaining register layouts and magic numbers, since these can't be intuited.

4. **Keep comments updated** — Wrong comments are worse than no comments.
