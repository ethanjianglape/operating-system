/* Linker script for limine higher-half x86_64 (64-bit) kernel */

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS
{
    text   PT_LOAD FLAGS(5); /* rx */
    ordata PT_LOAD FLAGS(4); /* r  */
    data   PT_LOAD FLAGS(6); /* rw */
}

SECTIONS
{
    /* Limine loads the kernel at this virtual address */
    KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;

    . = KERNEL_VIRT_BASE;
    kernel_start = .;

    .text ALIGN(4K) : {
        *(.text .text.*)
    } :text

    .rodata ALIGN(4K) : {
        *(.rodata .rodata.*)
    } :rodata

    .limine_requests ALIGN(4K) : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :rodata

    .data ALIGN(4K) : {
        *(.data .data.*)
    } :data

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    . = ALIGN(4K)
    kernel_end = .
}
