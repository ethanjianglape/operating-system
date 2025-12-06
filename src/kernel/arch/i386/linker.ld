/* Linker script for i386 (32-bit) kernel */

ENTRY(_start)

SECTIONS
{
    /* Kernel loads at 2MB physical address */
    . = 2M;

    /* Multiboot header section (must be at the beginning) */
    .boot :
    {
        *(.multiboot)
    }

    /* Read-only sections */
    .text ALIGN(4K) : AT(ADDR(.text))
    {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata))
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Read-write sections */
    .data ALIGN(4K) : AT(ADDR(.data))
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss))
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Stack section */
    .stack ALIGN(16) : AT(ADDR(.stack))
    {
        . = . + 16K; /* 16KB stack */
        stack_top = .;
    }

    /* Define kernel end */
    kernel_end = .;
}
