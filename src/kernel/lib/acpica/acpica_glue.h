/**
 * @file acpica_glue.h
 * @brief C/C++ glue layer for ACPICA OS Services Layer
 *
 * This header declares extern "C" functions that bridge ACPICA's C code
 * to the kernel's C++ APIs. The OSL implementation (acpica_osl.c) calls
 * these functions, which are implemented in acpica_glue.cpp.
 */

#ifndef ACPICA_GLUE_H
#define ACPICA_GLUE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Memory allocation - wraps kmalloc/kfree */
void* acpica_glue_alloc(unsigned long size);
void acpica_glue_free(void* ptr);

/* Memory mapping - maps physical address via HHDM */
void* acpica_glue_map_physical(unsigned long phys_addr, unsigned long size);

/* I/O port access */
unsigned char acpica_glue_inb(unsigned short port);
void acpica_glue_outb(unsigned short port, unsigned char value);
unsigned short acpica_glue_inw(unsigned short port);
void acpica_glue_outw(unsigned short port, unsigned short value);
unsigned int acpica_glue_inl(unsigned short port);
void acpica_glue_outl(unsigned short port, unsigned int value);

/* HHDM offset for direct physical-to-virtual conversion */
unsigned long acpica_glue_get_hhdm_offset(void);

#ifdef __cplusplus
}
#endif

#endif /* ACPICA_GLUE_H */
