#ifndef __EPT_H__
#define __EPT_H__

#include <ntddk.h>

#include "MTRR.h"

#include "Utils.h"

#define PDE_2MB_REF                 0x80
#define PAGE_TABLE_ENTRY_COUNT      (PAGE_SIZE / sizeof(UINT64))
#define EPT_CACHED_PT_COUNT         20
#define EPT_PAGE_PRESENT_MASK       7

C_ASSERT( EPT_CACHED_PT_COUNT < 255 );

/*
 * To verify this is actually how the processor interprets guest-physical
 *  addresses, see [28.2.2] "EPT Translation Mechanism" (or the simplified
 *  and color coded document arranged by yours truly in the research folder
 *  of this project under the same name)
 */
typedef VA_LAYOUT GUEST_PA_LAYOUT;

/*
 *  Note: for each of the paging structures below, it's very important
 *   they are initialized to zero; as almost all instances of the reserved bits
 *   are required to be set to zero. Additionally, the unused bits of each
 *   structure's `BaseAddress` must be set to zero.
 */



// [28.2] "The Extended Page Table Mechanism (EPT)", [28.2.2] "EPT Translation Mechanism"



// [28.2.6.1] "Memory Type Used for Accessing EPT Paging Structures"
// [28.2.6.2] "Memory Type Used for Translated Guest-Physical Addresses"
typedef enum _EPT_MEM_TYPE
{
    EPT_MEM_UC = 0,                         // Memory is always uncacheable if CR0.CD is set
    EPT_MEM_WC = 1,                         // May be unsupported
    EPT_MEM_WT = 4,                         // May be unsupported
    EPT_MEM_WP = 5,                         // May be unsupported
    EPT_MEM_WB = 6                          // This is the default memory type, and should always be used unless there is a special case for UC memory
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
        UINT64 MemType : 3;                 // 0-2
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
        UINT64 ExecuteAccess : 1;           // 2            // If the "mode-based execute control for EPT" bit is set (VMCS), this controls ring 0 execution privileges, otherwise it controls general execution privileges
        UINT64 Reserved0 : 5;               // 3-7
        UINT64 Accessed : 1;                // 8            // If the "AccessedDirty" bit is set (EPTP), this bit specifies whether or not software has accessed the 512GB region controlled by this entry; ignored it is otherwise
        UINT64 Ignored0 : 1;                // 9
        UINT64 UserExecuteAccess : 1;       // 10           // If the "mode-based execute control for EPT" bit is set (VMCS), this controls ring 3 execution privileges, otherwise this bit is ignored
        UINT64 Ignored1 : 1;                // 11
        UINT64 BaseAddress : 40;            // 12-51        // Physical address which points to the base of a 4KB region comprised of 64 PDPTEs (seen below)
        UINT64 Ignored2 : 12;               // 52-63
    };
    UINT64 All;
} EPT_PML4E, *PEPT_PML4E;

// [Table 28-2] "Format of an EPT Page-Directory-Pointer-Table Entry (PDPTE) that Maps a 1-GByte Page"
//  Note, we've chosen to exclude this structure from the header, as it isn't used in our case

// [Table 28-3] "Format of an EPT Page-Directory-Pointer-Table Entry (PDPTE) that References an EPT Page Directory"
typedef union _EPT_PDPT_ENTRY
{
    struct
    {
        UINT64 ReadAccess : 1;              // 0
        UINT64 WriteAccess : 1;             // 1
        UINT64 ExecuteAccess : 1;           // 2            // Same as the above
        UINT64 Reserved0 : 5;               // 3-7
        UINT64 Accessed : 1;                // 8            // If the "AccessedDirty" bit (6) of the EPTP is set, this bit specifies whether or not software as accessed the 1GB region controlled by this entry (only valid if the 'large page' bit (7) is set); ignored otherwie
        UINT64 Ignored0 : 1;                // 9
        UINT64 UserExecuteAccess : 1;       // 10           // Same as the above
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
        UINT64 ExecuteAccess : 1;           // 2
        UINT64 MemType : 3;                 // 3-5          // [28.2.6]
        UINT64 IgnorePAT : 1;               // 6            // [28.2.6]
        UINT64 Ref2MBPage : 1;              // 7            // This must be set, otherwise this points to an EPT page table
        UINT64 Accessed : 1;                // 8
        UINT64 Dirty : 1;                   // 9            // Same as the above, except that this bit specifies written instead of accessed
        UINT64 UserExecuteAccess : 1;       // 10
        UINT64 Ignored0 : 1;                // 11
        UINT64 Reserved0 : 9;               // 12-20
        UINT64 PageAddress : 31;            // 21-51        // Physical address which points to the base of an actual 2MB page of memory // Note: only 27-bits are required here, as this must point to an allocaiton resting on a 2MB boundary (indexable via 21 bits)
        UINT64 Ignored1 : 11;               // 52-62
        UINT64 SuppressVE : 1;              // 63           // [Table 28-4], bit 63
    } Ref2MB;

    // [Table 28-5] "Format of an EPT Page-Directory Entry (PDE) that References an EPT Page Table"
    struct
    {
        UINT64 ReadAccess : 1;              // 0
        UINT64 WriteAccess : 1;             // 1
        UINT64 ExecuteAccess : 1;           // 2
        UINT64 Reserved0 : 4;               // 3-6
        UINT64 Ref2MBPage : 1;              // 7            // This must be unset, otherwise this points to a 2MB page
        UINT64 Accessed : 1;                // 8
        UINT64 Ignored0 : 1;                // 9
        UINT64 UserExecuteAccess : 1;       // 10
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
        UINT64 ExecuteAccess : 1;           // 2
        UINT64 MemType : 3;                 // 3-5
        UINT64 IgnorePAT : 1;               // 6
        UINT64 Ignored0 : 1;                // 7
        UINT64 Accessed : 1;                // 8
        UINT64 Dirty : 1;                   // 9
        UINT64 UserExecuteAccess : 1;       // 10
        UINT64 Ignored1 : 1;                // 11
        UINT64 BaseAddress : 40;            // 12-51
        UINT64 Ignored2 : 11;               // 52-62
        UINT64 SuppressVE : 1;              // 63
    };
    UINT64 All;
} EPT_PTE, *PEPT_PTE;

// Our custom structure built to interface with the generic
//  fields present across all EPT paging structures
typedef union _EPT_PAGE_ENTRY_GENERIC
{
    struct
    {
        UINT64 Present : 3;                 // 0            // If this value is greater than zero, the page is present (has R/W/X bits)
        UINT64 Reserved0 : 4;               // 3-6          // This value will signify pages that map data instead of tables (an additional altitude component is required to support the detection of PTEs passed as these generic pages)
        UINT64 LargePage : 1;               // 7            // If this bit is set, several of the surrounding fields may be of the incorrect size or not apply altogether
        UINT64 Reserved1 : 4;               // 8-11
        UINT64 BaseAddress : 40;            // 12-51
        UINT64 Reserved2 : 12;              // 52-63
    };
    struct
    {
        // Primarily to support compatability with generic pages which
        //  map actual data (large page PDPTEs (1GB mappings), large page PDEs (2MB mappings), and PTEs (4KB mappings))
        UINT64 ReadAccess : 1;              // 0
        UINT64 WriteAccess : 1;             // 1
        UINT64 ExecuteAccess : 1;           // 2
        UINT64 MemType : 3;                 // 3-5
    };
    UINT64 All;
} EPT_GENERIC_PAGE, *PEPT_GENERIC_PAGE;

#pragma warning(pop)



/*
 * This structure is used to hold cached
 *  page tables (PTs), which will be used
 *  when we are lacking additional kernel
 *  components to replace 2MB large page
 *  PDEs with PDEs pointing to PTs mapping
 *  4KB pages.
 */
typedef struct _EPT_CACHED_PT
{
    BOOLEAN Available;
    PEPT_PTE PageTable;
    UINT64 PageTablePA;
} EPT_CACHED_PT, *PEPT_CACHED_PT;

extern EPT_CACHED_PT g_pCachedPTList[EPT_CACHED_PT_COUNT];



// "INVEPT � Invalidate Translations Derived from EPT"
typedef enum __INVEPT_TYPE
{
    INVEPT_TYPE_SINGLE_CONTEXT = 1,
    INVEPT_TYPE_GLOBAL_CONTEXT
} INVEPT_TYPE;


//
// Global definitions
//

extern EPTP g_EPTP;



//
// Function definitions
//

// Assembly intrinsic (see './asm/eptintrin.asm')
//  NOTE: this function should not be called on it's own!
//  instead, use the EptInvalidateTlb function below!
VOID
__invept(
    CONST UINT64 Type,
    CONST UINT64 Eptp
    );

// Initializes the EPTP by allocating a PML4 table, and sets up
//  the appropriate flags for registration with the VMCS.
BOOLEAN
EptBuild();

/*
 * Initializes a cache of PTs that will be used to replace
 *  2MB large page PDEs. A cache is primarily used because
 *  of our need to convert large page PDEs into PDEs mapping
 *  PTs from within our VMM, which is at an IRQL which won't
 *  support OS memory mapping routines.
 */
VOID
EptInitializeCache();

// Used to invalidate EPT entries in the current logical processor's TLB
BOOLEAN
EptInvalidateTlb(
    CONST INVEPT_TYPE Type
    );

/*
 * This function allows us to selectively split 2MB large page 
 *  PDEs into PDEs which point to PTs holding 512 PTEs mapping
 *  4KB allocations. This preserves the underlying identity
 *  mapping, and supports using a cache of PTs to allow for
 *  selective splitting from without our VMM.
 */
BOOLEAN
EptConvertLargePagePde(
    _In_ CONST BOOLEAN UseCache,
    _Inout_ PEPT_PDE CONST Pde,
    _Out_opt_ PEPT_PTE* CONST Pt
    );

/*
 * This will map 64GB of physical memory (which we assume is
 *  the most a system should ever have loaded) in a way that
 *  is indexable via our EPT.
 *
 * This routine will also utilize the newly-added MTRR source
 *  files to ensure the correct memory types are set for the
 *  corresponding pages.
 *
 * Note: this is considerably simpler than having to insert
 *  just one 4KB EPT entry for a provided virtual address.
 */
BOOLEAN
EptIdentityMapSystem();

// This simply translates a physical address through the EPT
//  and into an EPT PTE. 2MB large page PDEs are automatically split.
BOOLEAN
EptGetPte(
    _In_ CONST BOOLEAN VmmRequest,
    _In_ CONST PHYSICAL_ADDRESS TargetPa,
    _Inout_ PEPT_PTE* CONST Pte
    );

/*
 * Indexes our EPT table, and removes every allocation (including deallocating
 *  the page tables we allocated when inserting addresses into the EPT).
 * Additionally, this function has been fitted to account for the deallocation
 *  of cached PTs as well.
 */
BOOLEAN
EptTeardown();

#endif // __EPT_H__
