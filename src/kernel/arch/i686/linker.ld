/* Linker script for higher-half i686 (32-bit) kernel */

ENTRY(_start)

PHDRS
{
    boot PT_LOAD FLAGS(7);
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    /* Kernel loads at 2MB physical address, runs at 3GB + 2MB virtual */
    KERNEL_VIRT_BASE = 0xC0000000; /* 3 GB */
    KERNEL_PHYS_BASE = 0x00200000; /* 2 MB */

    /* Boot section at physical address for GRUB and early boot) */
    . = KERNEL_PHYS_BASE;

    /* Multiboot header section (must be at the beginning) */
    .boot ALIGN(4K) : AT(ADDR(.boot))
    {
        *(.multiboot)
    } :boot

    /* Early boot code that runs before paging */
    .boot.text ALIGN(4K) : AT(ADDR(.boot.text))
    {
        *(.boot.text)
    } :boot

    /* Early boot data (GDT, page tables) at low addresses */
    .boot.data ALIGN(4K) : AT(ADDR(.boot.data))
    {
        *(.boot.data)
    } :boot

    /* Switch to higher-half for kernel_main() */
    . += KERNEL_VIRT_BASE;

    /* Read-only sections */
    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRT_BASE)
    {
        *(.text)
        *(.text.*)
    } :text

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE)
    {
        *(.rodata)
        *(.rodata.*)
    } :text

    /* Read-write sections */
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRT_BASE)
    {
        *(.data)
        *(.data.*)
    } :data

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRT_BASE)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    } :data

    /* Stack section */
    .stack ALIGN(16) : AT(ADDR(.stack) - KERNEL_VIRT_BASE)
    {
        . = . + 64K; /* 64KB stack */
        stack_top = .;
    } :data

    /* Define kernel end */
    kernel_end = .;
    kernel_physical_end = . - KERNEL_VIRT_BASE;
}
