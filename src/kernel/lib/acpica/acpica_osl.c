/**
 * @file acpica_osl.c
 * @brief ACPICA OS Services Layer (OSL) implementation for MyOS
 *
 * This file implements the OS-specific functions required by ACPICA.
 * Many functions are stubs for initial build - they will be fleshed out
 * as needed for actual ACPI functionality.
 */

#include "acpi.h"
#include "acpica_glue.h"

/* Stored RSDP address from Limine bootloader */
static ACPI_PHYSICAL_ADDRESS g_rsdp_address = 0;

/**
 * Set the RSDP address (call before AcpiInitializeSubsystem)
 */
void AcpiOslSetRsdp(ACPI_PHYSICAL_ADDRESS addr) {
    g_rsdp_address = addr;
}

/* ==========================================================================
 * Initialization and Termination
 * ========================================================================== */

ACPI_STATUS AcpiOsInitialize(void) {
    return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void) {
    return AE_OK;
}

/* ==========================================================================
 * ACPI Table Interfaces
 * ========================================================================== */

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void) {
    return g_rsdp_address;
}

ACPI_STATUS AcpiOsPredefinedOverride(
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING *NewVal) {

    (void)InitVal;
    *NewVal = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(
    ACPI_TABLE_HEADER *ExistingTable,
    ACPI_TABLE_HEADER **NewTable) {

    (void)ExistingTable;
    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(
    ACPI_TABLE_HEADER *ExistingTable,
    ACPI_PHYSICAL_ADDRESS *NewAddress,
    UINT32 *NewTableLength) {

    (void)ExistingTable;
    *NewAddress = 0;
    *NewTableLength = 0;
    return AE_OK;
}

/* ==========================================================================
 * Spinlock Primitives (single-threaded stubs)
 * ========================================================================== */

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
    *OutHandle = (ACPI_SPINLOCK)(ACPI_SIZE)1; /* Non-NULL dummy */
    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) {
    (void)Handle;
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
    (void)Handle;
    return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {
    (void)Handle;
    (void)Flags;
}

/* ==========================================================================
 * Semaphore Primitives (single-threaded stubs)
 * ========================================================================== */

ACPI_STATUS AcpiOsCreateSemaphore(
    UINT32 MaxUnits,
    UINT32 InitialUnits,
    ACPI_SEMAPHORE *OutHandle) {

    (void)MaxUnits;
    (void)InitialUnits;
    *OutHandle = (ACPI_SEMAPHORE)(ACPI_SIZE)1;
    return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
    (void)Handle;
    return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(
    ACPI_SEMAPHORE Handle,
    UINT32 Units,
    UINT16 Timeout) {

    (void)Handle;
    (void)Units;
    (void)Timeout;
    return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
    (void)Handle;
    (void)Units;
    return AE_OK;
}

/* ==========================================================================
 * Memory Allocation and Mapping
 * ========================================================================== */

void *AcpiOsAllocate(ACPI_SIZE Size) {
    return acpica_glue_alloc((unsigned long)Size);
}

/* Note: AcpiOsAllocateZeroed is provided by ACPICA's utalloc.c */

void AcpiOsFree(void *Memory) {
    acpica_glue_free(Memory);
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS Where, ACPI_SIZE Length) {
    /* Use HHDM - physical addresses are directly mapped at hhdm_offset + phys */
    unsigned long hhdm = acpica_glue_get_hhdm_offset();
    (void)Length; /* All memory is mapped via HHDM */
    return (void *)(hhdm + Where);
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Size) {
    /* HHDM mappings persist, no unmap needed */
    (void)LogicalAddress;
    (void)Size;
}

ACPI_STATUS AcpiOsGetPhysicalAddress(
    void *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS *PhysicalAddress) {

    /* Reverse the HHDM mapping */
    unsigned long hhdm = acpica_glue_get_hhdm_offset();
    *PhysicalAddress = (ACPI_PHYSICAL_ADDRESS)LogicalAddress - hhdm;
    return AE_OK;
}

/* ==========================================================================
 * Object Cache
 * Note: ACPICA provides cache functions in utcache.c when ACPI_USE_LOCAL_CACHE
 * is defined (which we do in acmyos.h). No need to implement here.
 * ========================================================================== */

/* ==========================================================================
 * Interrupt Handlers (not implemented - stubs)
 * ========================================================================== */

ACPI_STATUS AcpiOsInstallInterruptHandler(
    UINT32 InterruptNumber,
    ACPI_OSD_HANDLER ServiceRoutine,
    void *Context) {

    (void)InterruptNumber;
    (void)ServiceRoutine;
    (void)Context;
    return AE_OK; /* Pretend success for now */
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(
    UINT32 InterruptNumber,
    ACPI_OSD_HANDLER ServiceRoutine) {

    (void)InterruptNumber;
    (void)ServiceRoutine;
    return AE_OK;
}

/* ==========================================================================
 * Threads and Scheduling
 * ========================================================================== */

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
    return 1; /* Single thread */
}

ACPI_STATUS AcpiOsExecute(
    ACPI_EXECUTE_TYPE Type,
    ACPI_OSD_EXEC_CALLBACK Function,
    void *Context) {

    (void)Type;
    /* In single-threaded mode, just execute synchronously */
    if (Function) {
        Function(Context);
    }
    return AE_OK;
}

void AcpiOsWaitEventsComplete(void) {
    /* No async events in single-threaded mode */
}

void AcpiOsSleep(UINT64 Milliseconds) {
    /* Simple busy-wait for now - TODO: use kernel sleep */
    volatile UINT64 i;
    for (i = 0; i < Milliseconds * 1000; i++) {
        __asm__ __volatile__("pause");
    }
}

void AcpiOsStall(UINT32 Microseconds) {
    /* Busy-wait */
    volatile UINT32 i;
    for (i = 0; i < Microseconds * 10; i++) {
        __asm__ __volatile__("pause");
    }
}

/* ==========================================================================
 * I/O Port Access
 * ========================================================================== */

ACPI_STATUS AcpiOsReadPort(
    ACPI_IO_ADDRESS Address,
    UINT32 *Value,
    UINT32 Width) {

    switch (Width) {
        case 8:
            *Value = acpica_glue_inb((unsigned short)Address);
            break;
        case 16:
            *Value = acpica_glue_inw((unsigned short)Address);
            break;
        case 32:
            *Value = acpica_glue_inl((unsigned short)Address);
            break;
        default:
            return AE_BAD_PARAMETER;
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(
    ACPI_IO_ADDRESS Address,
    UINT32 Value,
    UINT32 Width) {

    switch (Width) {
        case 8:
            acpica_glue_outb((unsigned short)Address, (unsigned char)Value);
            break;
        case 16:
            acpica_glue_outw((unsigned short)Address, (unsigned short)Value);
            break;
        case 32:
            acpica_glue_outl((unsigned short)Address, Value);
            break;
        default:
            return AE_BAD_PARAMETER;
    }
    return AE_OK;
}

/* ==========================================================================
 * Physical Memory Read/Write
 * ========================================================================== */

ACPI_STATUS AcpiOsReadMemory(
    ACPI_PHYSICAL_ADDRESS Address,
    UINT64 *Value,
    UINT32 Width) {

    void *virt = AcpiOsMapMemory(Address, Width / 8);

    switch (Width) {
        case 8:
            *Value = *(volatile UINT8 *)virt;
            break;
        case 16:
            *Value = *(volatile UINT16 *)virt;
            break;
        case 32:
            *Value = *(volatile UINT32 *)virt;
            break;
        case 64:
            *Value = *(volatile UINT64 *)virt;
            break;
        default:
            return AE_BAD_PARAMETER;
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(
    ACPI_PHYSICAL_ADDRESS Address,
    UINT64 Value,
    UINT32 Width) {

    void *virt = AcpiOsMapMemory(Address, Width / 8);

    switch (Width) {
        case 8:
            *(volatile UINT8 *)virt = (UINT8)Value;
            break;
        case 16:
            *(volatile UINT16 *)virt = (UINT16)Value;
            break;
        case 32:
            *(volatile UINT32 *)virt = (UINT32)Value;
            break;
        case 64:
            *(volatile UINT64 *)virt = Value;
            break;
        default:
            return AE_BAD_PARAMETER;
    }
    return AE_OK;
}

/* ==========================================================================
 * PCI Configuration Space (not implemented - stubs)
 * ========================================================================== */

ACPI_STATUS AcpiOsReadPciConfiguration(
    ACPI_PCI_ID *PciId,
    UINT32 Reg,
    UINT64 *Value,
    UINT32 Width) {

    (void)PciId;
    (void)Reg;
    (void)Width;
    *Value = 0;
    return AE_SUPPORT; /* Not supported yet */
}

ACPI_STATUS AcpiOsWritePciConfiguration(
    ACPI_PCI_ID *PciId,
    UINT32 Reg,
    UINT64 Value,
    UINT32 Width) {

    (void)PciId;
    (void)Reg;
    (void)Value;
    (void)Width;
    return AE_SUPPORT; /* Not supported yet */
}

/* ==========================================================================
 * Miscellaneous
 * ========================================================================== */

BOOLEAN AcpiOsReadable(void *Pointer, ACPI_SIZE Length) {
    (void)Pointer;
    (void)Length;
    return TRUE; /* Assume all mapped memory is readable */
}

BOOLEAN AcpiOsWritable(void *Pointer, ACPI_SIZE Length) {
    (void)Pointer;
    (void)Length;
    return TRUE; /* Assume all mapped memory is writable */
}

UINT64 AcpiOsGetTimer(void) {
    /* Return 0 for now - TODO: implement with TSC or APIC timer */
    /* Timer should return 100-nanosecond units */
    return 0;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
    (void)Function;
    (void)Info;
    return AE_OK;
}

ACPI_STATUS AcpiOsEnterSleep(
    UINT8 SleepState,
    UINT32 RegaValue,
    UINT32 RegbValue) {

    (void)SleepState;
    (void)RegaValue;
    (void)RegbValue;
    return AE_OK;
}

/* ==========================================================================
 * Debug Print Routines
 * ========================================================================== */

void AcpiOsPrintf(const char *Format, ...) {
    /* Stub - TODO: forward to kernel logging */
    (void)Format;
}

void AcpiOsVprintf(const char *Format, va_list Args) {
    /* Stub - TODO: forward to kernel logging */
    (void)Format;
    (void)Args;
}

void AcpiOsRedirectOutput(void *Destination) {
    (void)Destination;
}

/* ==========================================================================
 * Debug IO (stubs)
 * ========================================================================== */

ACPI_STATUS AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 *BytesRead) {
    (void)Buffer;
    (void)BufferLength;
    if (BytesRead) {
        *BytesRead = 0;
    }
    return AE_SUPPORT;
}

ACPI_STATUS AcpiOsInitializeDebugger(void) {
    return AE_SUPPORT;
}

void AcpiOsTerminateDebugger(void) {
}

ACPI_STATUS AcpiOsWaitCommandReady(void) {
    return AE_SUPPORT;
}

ACPI_STATUS AcpiOsNotifyCommandComplete(void) {
    return AE_SUPPORT;
}

void AcpiOsTracePoint(
    ACPI_TRACE_EVENT_TYPE Type,
    BOOLEAN Begin,
    UINT8 *Aml,
    char *Pathname) {

    (void)Type;
    (void)Begin;
    (void)Aml;
    (void)Pathname;
}

/* ==========================================================================
 * Table Access (stubs - tables accessed via standard ACPICA methods)
 * ========================================================================== */

ACPI_STATUS AcpiOsGetTableByName(
    char *Signature,
    UINT32 Instance,
    ACPI_TABLE_HEADER **Table,
    ACPI_PHYSICAL_ADDRESS *Address) {

    (void)Signature;
    (void)Instance;
    (void)Table;
    (void)Address;
    return AE_SUPPORT;
}

ACPI_STATUS AcpiOsGetTableByIndex(
    UINT32 Index,
    ACPI_TABLE_HEADER **Table,
    UINT32 *Instance,
    ACPI_PHYSICAL_ADDRESS *Address) {

    (void)Index;
    (void)Table;
    (void)Instance;
    (void)Address;
    return AE_SUPPORT;
}

ACPI_STATUS AcpiOsGetTableByAddress(
    ACPI_PHYSICAL_ADDRESS Address,
    ACPI_TABLE_HEADER **Table) {

    (void)Address;
    (void)Table;
    return AE_SUPPORT;
}

/* ==========================================================================
 * Directory Operations (stubs - not needed for embedded use)
 * ========================================================================== */

void *AcpiOsOpenDirectory(char *Pathname, char *WildcardSpec, char RequestedFileType) {
    (void)Pathname;
    (void)WildcardSpec;
    (void)RequestedFileType;
    return NULL;
}

char *AcpiOsGetNextFilename(void *DirHandle) {
    (void)DirHandle;
    return NULL;
}

void AcpiOsCloseDirectory(void *DirHandle) {
    (void)DirHandle;
}
