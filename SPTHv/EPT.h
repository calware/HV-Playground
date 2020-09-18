#ifndef __EPT_H__
#define __EPT_H__

#include "Memory.h"

#include "Utils.h"

#define PDE_2MB_REF                 0x80
#define PAGE_TABLE_ENTRY_COUNT      (PAGE_SIZE / sizeof(UINT64))

/*
 * To verify this is actually how the processor interprets guest-physical
 *  addresses, see [28.2.2] "EPT Translation Mechanism" (or the simplified
 *  and color coded document arranged by yours truly in the research folder
 *  of this project)
 */
typedef VA_LAYOUT GUEST_PA_LAYOUT;

/*
 *  Note: for each of the paging structures below, it's very important
 *   they are initialized to zero; as most instances of the reserved bits
 *   are required to be set to zero. Additionally, the unused bits of each
 *   structure's BaseAddress must be set to zero.
 */

// [28.2] "The Extended Page Table Mechanism (EPT)", [28.2.2] "EPT Translation Mechanism"



// [28.2.6.1] "Memory Type Used for Accessing EPT Paging Structures"
// [28.2.6.2] "Memory Type Used for Translated Guest-Physical Addresses"
typedef enum _EPT_MEM_TYPE
{
    EPT_MEM_UC = 0,                         // Memory is always uncacheable if CR0.CD is set
    EPT_MEM_WC = 1,
    EPT_MEM_WT = 4,
    EPT_MEM_WP = 5,
    EPT_MEM_WB = 6
}EPT_MEM_TYPE;

// Private enumeration set used to denote the current paging altitude value
//  seen in our functions below
typedef enum _EPT_ALTITUDE
{
    EPT_ALTITUDE_PT = 1,
    EPT_ALTITUDE_PD,
    EPT_ALTITUDE_PDPT,
    EPT_ALTITUDE_PML4
} EPT_ALTITUDE;

#pragma warning(push)

#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int

// [24.6.11] "Extended-Page-Table Pointer (EPTP)
typedef union _EPT_POINTER
{
    struct
    {
        UINT64 MemType : 3;                 // 0-2          // pretty sure this has to be WB if CR0.CD is unset
        UINT64 WalkLength : 3;              // 3-5
        UINT64 EnableAccessedDirty : 1;     // 6
        UINT64 Reserved0 : 5;               // 7-11
        UINT64 BaseAddress : 40;            // 12-51        // Physical address which points to the base of a 4KB region comprised of 64 PML4Es (seen below)
        UINT64 Reserved1 : 12;              // 52-63
    };
    UINT64 All;
} EPTP, *PEPTP;

// [Table 28-1] "Format of an EPT PML4 Entry (PML4E) that References an EPT Page-Directory-Pointer Table"
typedef union _EPT_PML4_ENTRY
{
    struct
    {
        UINT64 ReadAccess : 1;              // 0
        UINT64 WriteAccess : 1;             // 1
        UINT64 ExecuteAccess : 1;           // 2            // if the "mode-based execute control for EPT" bit is set (VMCS), this controls ring 0 execution privileges, otherwise it controls general execution privileges
        UINT64 Reserved0 : 5;               // 3-7
        UINT64 Accessed : 1;                // 8            // if the "AccessedDirty" bit is set (EPTP), this bit specifies whether or not software has accessed the 512GB region controlled by this entry; ignored otherwise
        UINT64 Ignored0 : 1;                // 9
        UINT64 UserExecuteAccess : 1;       // 10           // if the "mode-based execute control for EPT" bit is set (VMCS), this controls ring 3 execution privileges, otherwise this bit is ignored
        UINT64 Ignored1 : 1;                // 11
        UINT64 BaseAddress : 40;            // 12-51        // Physical address which points to the base of a 4KB region comprised of 64 PDPTEs (seen below)
        UINT64 Ignored2 : 12;               // 52-63
    };
    UINT64 All;
} EPT_PML4E, *PEPT_PML4E;

// [Table 28-2] "Format of an EPT Page-Directory-Pointer-Table Entry (PDPTE) that Maps a 1-GByte Page"

// [Table 28-3] "Format of an EPT Page-Directory-Pointer-Table Entry (PDPTE) that References an EPT Page Directory"
typedef union _EPT_PDPT_ENTRY
{
    struct
    {
        UINT64 ReadAccess : 1;              // 0
        UINT64 WriteAccess : 1;             // 1
        UINT64 ExecuteAccess : 1;           // 2            // " "
        UINT64 Reserved0 : 5;               // 3-7
        UINT64 Accessed : 1;                // 8            // if the "AccessedDirty" bit (6) of the EPTP is set, this bit specifies whether or not software as accessed the 1GB region controlled by this entry; ignored otherwie
        UINT64 Ignored0 : 1;                // 9
        UINT64 UserExecuteAccess : 1;       // 10           // " "
        UINT64 Ignored1 : 1;                // 11
        UINT64 BaseAddress : 40;            // 12-51        // Physical address which points to the base of a 4KB region comprised of 64 PDEs
        UINT64 Ignored2 : 12;               // 52-63
    };
    UINT64 All;
} EPT_PDPTE, *PEPT_PDPTE;

typedef union _EPT_PD_ENTRY
{
    // [Table 28-4] "Format of an EPT Page-Directory Entry (PDE) that Maps a 2-MByte Page"
    struct
    {
        UINT64 ReadAccess : 1;              // 0
        UINT64 WriteAccess : 1;             // 1
        UINT64 ExecuteAccess : 1;           // 2            // " "
        UINT64 MemType : 3;                 // 3-5          // [28.2.6]
        UINT64 IgnorePAT : 1;               // 6            // [28.2.6]
        UINT64 Ref2MBPage : 1;              // 7            // This must be set to 1, otherwise this points to an EPT page table
        UINT64 Accessed : 1;                // 8            // if the "AccessedDirty" bit (6) of the EPTP is set, this bit specifies whether or not software as accessed the 2MB region controlled by this entry; ignored otherwie
        UINT64 Dirty : 1;                   // 9            // same as the above except that this bit specifies written instead of accessed
        UINT64 UserExecuteAccess : 1;       // 10           // " "
        UINT64 Ignored0 : 1;                // 11
        UINT64 Reserved0 : 9;               // 12-20
        UINT64 PageAddress : 31;            // 21-51        // Physical address which points to the base of an actual 2MB page of memory // Note: only 27-bits are required here, as this must point to an allocaiton resting on a 2MB boundary (indexable via 21 bits)
        UINT64 Ignored1 : 11;               // 52-62
        UINT64 SuppressVE : 1;              // 63           // EPT violations caused by accesses to this page are convertible to virtualization exceptions (only if this bit is zero). if "EPT-violation #VE" (VMCS) control is zero, this bit is ignored
    } Ref2MB;

    // [Table 28-5] "Format of an EPT Page-Directory Entry (PDE) that References an EPT Page Table"
    struct
    {
        UINT64 ReadAccess : 1;              // 0
        UINT64 WriteAccess : 1;             // 1
        UINT64 ExecuteAccess : 1;           // 2            // " "
        UINT64 Reserved0 : 4;               // 3-6
        UINT64 Ref2MBPage : 1;              // 7            // This must be set to 0, otherwise this points to a 2MB page
        UINT64 Accessed : 1;                // 8            // Same as the above, except still for 2MB pages
        UINT64 Ignored0 : 1;                // 9
        UINT64 UserExecuteAccess : 1;       // 10           // " "
        UINT64 Ignored1 : 1;                // 11
        UINT64 BaseAddress : 40;            // 12-51        // Physical address which points to the base of a 4KB region comprised of 64 PTEs
        UINT64 Ignored2 : 12;               // 52-63
    } RefPT;

    UINT64 All;
} EPT_PDE, *PEPT_PDE;



// [Table 28-6] "Format of an EPT Page-Table Entry that Maps a 4-KByte Page"
typedef union _EPT_PT_ENTRY
{
    struct
    {
        UINT64 ReadAccess : 1;              // 0
        UINT64 WriteAccess : 1;             // 1
        UINT64 ExecuteAccess : 1;           // 2            // " "
        UINT64 MemType : 3;                 // 3-5          // if CR0.CD set, then memory type is UC, else UC or WB
        UINT64 IgnorePAT : 1;               // 6
        UINT64 Ignored0 : 1;                // 7
        UINT64 Accessed : 1;                // 8            // " "
        UINT64 Dirty : 1;                   // 9            // " "
        UINT64 UserExecuteAccess : 1;       // 10           // " "
        UINT64 Ignored1 : 1;                // 11
        UINT64 BaseAddress : 40;            // 12-51
        UINT64 Ignored2 : 11;               // 52-62
        UINT64 SuppressVE : 1;              // 63
    };
    UINT64 All;
} EPT_PTE, *PEPT_PTE;

typedef union _EPT_PAGE_ENTRY_GENERIC
{
    struct
    {
        UINT64 Present : 3;                 // 0            // if this value is greater than zero, the page is present (has R/W/X bits)
        UINT64 Reserved0 : 4;               // 3-6          // this value will signify pages that map data where we wouldn't have a LargePage bit
        UINT64 LargePage : 1;               // 7            // if this bit is set, several of the surrounding fields may be of the incorrect size or not apply altogether
        UINT64 Reserved1 : 4;               // 8-11
        UINT64 BaseAddress : 40;            // 12-51
        UINT64 Reserved2 : 12;              // 52-63
    };
    struct
    {
        // Primarily to support compatability with generic pages which
        //  map actual data (1GB PDPTEs, 2MB PDEs, 4KB PTEs)
        UINT64 ReadAccess : 1;
        UINT64 WriteAccess : 1;
        UINT64 ExecuteAccess : 1;
        UINT64 MemType : 3;
        // ...
    };
    UINT64 All;
} EPT_GENERIC_PAGE, *PEPT_GENERIC_PAGE;

#pragma warning(pop)



// [28.2.3] "EPT-Induced VM Exits"

// READ 3A pg.448 11.11 Memory Type Range Registers (MTRRS), which also follows into examples and how to use PAT!

// ...



//
// Global definitions
//

static EPTP g_EPTP;



//
// Function definitions
//

// Initializes the EPTP by allocating a PML4 table, and sets up
//  the appropriate flags for registration with the VMCS
BOOLEAN
BuildEPT();

/*
 * For each virtual address this function is passed, it will insert
 *  into the EPT every physical address required to index this virtual
 *  address, and point the final entry to the 4KB page containing the
 *  data for the initial virtual address passed as a parameter.
 *
 * This function is used to quickly and easily identity map a desired
 *  virtual address in a way in which it can be indexed using the
 *  appropriate guest-physical addresses.
 */
BOOLEAN
InsertEPTEntry(
    _In_ CONST PVOID VirtualAddress
    );
/*
 * Indexes our EPT table, and removes every allocation (including those used
 *  for the tables themselves).
 *
 * Note: this function calls an internal function which tears down the EPT by
 *  *deallocating* all of the entries which point to actual data; as each
 *  entry in our EPT was an allocation made via the `InsertEPTEntry` function
 *  above. For that reason, this function should not be used to deinitialize
 *  an EPT table which utilizes full identity mappings with the host.
 */
BOOLEAN
TeardownEPT();

#endif // __EPT_H__
