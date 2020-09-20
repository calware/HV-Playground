#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <ntddk.h>

#include <intrin.h>

#define PAGESIZE_BIT                    0x40
#define PAGE_OFFSET_4KB                 12
#define PAGING_TABLE_BASE_ADDRESS_MASK  0xFFFFFF000

// Note: this can only be used on tables which map 4KB regions
//  Pages which map 'large' (1GB/2MB) regions are not supported
#define TABLE_BASE_ADDRESS(a) ((UINT64)(a & PAGING_TABLE_BASE_ADDRESS_MASK))

#define LARGE_PAGE_MAPPING(a) ((a & PAGESIZE_BIT) == 1)

#pragma warning(push)

#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int

//
// Structural definitions
//

/*
 * Layout of an address used in a 4-level page walk
 *
 * Note: this could be either a standard virtual address, or a guest-physical address
 *
 * In the case of a guest-physical address, you may be concerned that your physical
 *  address width doesn't match (or exceed 48 bits), but is is completely fine and to be expected.
 * Say your physical address width is only 34 bits—to index only 16GB, you only need 34 bits
 *  (16*1024^3 = 2^34)—the bits above this point, that would be used to index the PML4, are just
 *  going to be zero; which is fine because each PML4E maps 512GB of memory, which you don't have.
 */
typedef union _VA_PA_LAYOUT
{
    struct
    {
        UINT64 Offset : 12;                 // 0-11
        UINT64 PTIndex : 9;                 // 12-20
        UINT64 PDIndex : 9;                 // 21-29
        UINT64 PDPTIndex : 9;               // 30-38
        UINT64 PML4Index : 9;               // 39-47
        UINT64 Ignored0 : 16;               // 48-63
    };
    UINT64 All;
} VA_LAYOUT;

// [Table 4-12] "Use of CR3 with 4-Level Paging and 5-level Paging and CR4.PCIDE = 0"
typedef union _CR3
{
    struct
    {
        UINT64 Ignored0 : 3;                // 0-2
        UINT64 PWT : 1;                     // 3 (PAT)
        UINT64 PCD : 1;                     // 4 (PAT)
        UINT64 Ignored1 : 7;                // 5-11
        UINT64 PhysicalAddress : 36;        // 12-47        // Allowing addressing of up to 64-bits on a 4-level page walk (this could be longer if we wanted—i.e. no need for reserved top bits)
        UINT64 Reserved0 : 16;              // 48-63
    };
    UINT64 All;
} CR3, PML4;

/*
 * [Vol.3A Table 4-15] "Format of a PML4 Entry (PML4E) that References a Page-Directory-Pointer Table"
 * [Vol.3A Table 4-17] "Format of a Page-Directory-Pointer-Table Entry (PDPTE) that References a Page Directory"
 * [Vol.3A Table 4-19] "Format of a Page-Directory Entry that References a Page Table"
 * [Vol.3A Table 4-20] "Format of a Page-Table Entry that Maps a 4-KByte Page"
 *
 * Note: this is the layout for a generic page table entry that maps a 4KB region—be that a page table, or
 *  actual data (in the case of a PTE). In the case of mappings exceeding 4KB in size (large page mappings
 *  of 1GB or 2MB) the fields are slightly different.
 */
typedef union _PAGING_ENTRY
{
    struct
    {
        UINT64 Present : 1;                 // 0
        UINT64 RW : 1;                      // 1
        UINT64 US : 1;                      // 2
        UINT64 PWT : 1;                     // 3
        UINT64 PCD : 1;                     // 4
        UINT64 Accessed : 1;                // 5
        UINT64 Dirty : 1;                   // 6
        UINT64 PAT : 1;                     // 7
        UINT64 G : 1;                       // 8
        UINT64 Ignored0 : 3;                // 9-11
        UINT64 PhysicalAddress : 36;        // 12-47        // " "
        UINT64 Reserved0 : 4;               // 48-51
        UINT64 Ignored1 : 7;                // 52-58
        UINT64 ProtectionKey : 4;           // 59-62
        UINT64 XD : 1;                      // 63
    };
    UINT64 All;
} PML4E, PDPTE, PDE, PTE, PAGING_ENTRY;

typedef struct _INDEX_ENTRY
{
    PAGING_ENTRY Fields;                    // It may be beneficial to know which permision bits are to be set on the page in question
    UINT64 BaseAddress;                     // This holds the base address of each page table, and is mainly what has to be translated with traversing our EPT paging tables
} INDEX_ENTRY;

/*
 * This data structure is utilized by our EPT source functions to
 *  insert a virtual address into the paging tables.
 *
 * Each entry's `BaseAddress` field must be individually indexable via our EPT
 *  tables (to facilitate standard VA->PA translations), so use the VA_LAYOUT
 *  fields of the `BaseAddress` of entries to index our own EPT page tables and
 *  then add the corresponding entry `BaseAddress` and the end of thepage walk.
 */
typedef struct _INDEX_POINTS
{
    INDEX_ENTRY PML4E;
    INDEX_ENTRY PDPTE;
    INDEX_ENTRY PDE;
    INDEX_ENTRY PTE;
    INDEX_ENTRY Page;
} INDEX_POINTS, *PINDEX_POINTS;

#pragma warning(pop)



//
// External definitions
//

// MmMapIoSpace won't allow us to map page tables, so we have
//  decided to use the undocumented MmGetVirtualForPhysical here
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(HIGH_LEVEL)
NTSYSAPI
PVOID
MmGetVirtualForPhysical(
    _In_ PHYSICAL_ADDRESS PhysicalAddress
    );



//
// Function definitions
//

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN
GetPhyscialIndexPoints(
    _In_ CONST PML4 PhysicalPML4,
    _In_ CONST PVOID VirtualAddress,
    _Out_ PINDEX_POINTS CONST IndexPoints
    );

#endif // __MEMORY_H__
