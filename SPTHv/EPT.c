#include "EPT.h"

BOOLEAN
BuildEPT()
{
    VMX_ADDRESS pml4Allocation;

    if ( utlAllocateVMXData(PAGE_SIZE, TRUE, TRUE, &pml4Allocation) == FALSE )
    {
        return FALSE;
    }

    g_EPTP.BaseAddress = (UINT64)pml4Allocation.PA;

    /*
     * [28.2.6.1] "Memory Type Used for Accessing EPT Paging Structures"
     * [A.1] "Basic VMX Information"
     *
     * No reason not to allow the processor to cache the EPTP
     */
    g_EPTP.MemType = EPT_MEM_WB;

    // [24.6.11] "Extended-Page-Table Pointer (EPTP)"
    //  "This value is 1 less than the EPT page-walk length..."
    // We want to indicate a 4-level page walk
    g_EPTP.WalkLength = 3;

    // We're not using the accessed or dirty flags in our EPT entries
    g_EPTP.EnableAccessedDirty = FALSE;

    return TRUE;
}

/*
 * Recursive function used internally to insert physical addresses
 *  into the EPT by traversing and, where required, allocating new
 *  tables
 */
BOOLEAN
InsertEPTIdentityMapping(
    _In_ CONST UINT8 Altitude,
    _In_ CONST UINT64 PhysicalPageTableBase,
    _In_ CONST UINT64 PhysicalAddress
    )
{
    BOOLEAN result;

    PVOID pageTableVA;
    PHYSICAL_ADDRESS pageTablePA;

    EPT_GENERIC_PAGE* pPageEntry;
    VMX_ADDRESS pageAllocation;

    GUEST_PA_LAYOUT targetPA;
    targetPA.All = PhysicalAddress;

    UINT16 pageIndex;

    // Obtain the index specific to this page table based on the provided altitude
    switch ( Altitude )
    {
        case EPT_ALTITUDE_PML4:
            pageIndex = targetPA.PML4Index;
            break;
        case EPT_ALTITUDE_PDPT:
            pageIndex = targetPA.PDPTIndex;
            break;
        case EPT_ALTITUDE_PD:
            pageIndex = targetPA.PDIndex;
            break;
        case EPT_ALTITUDE_PT:
            pageIndex = targetPA.PTIndex;
            break;
        default:
            NT_ASSERT( FALSE );
            break;
    }

    pageTablePA.QuadPart = 0;
    pageTablePA.QuadPart = PhysicalPageTableBase;

    pageTableVA = MmGetVirtualForPhysical(pageTablePA);

    NT_ASSERT( pageTableVA != NULL );

    pPageEntry = (EPT_GENERIC_PAGE*)pageTableVA;

    // Check if we need to insert data into the EPT
    if ( pPageEntry[pageIndex].All == 0 )
    {
        // Set every entry to have unrestricted access permissions, which
        //  will delegate out access checks to the underlying OS
        pPageEntry->ReadAccess    =
        pPageEntry->WriteAccess   =
        pPageEntry->ExecuteAccess = TRUE;
        
        if ( Altitude != EPT_ALTITUDE_PT )
        {
            // We're initializing a page table

            // Allocate our table
            NT_ASSERT(utlAllocateVMXData(PAGE_SIZE, TRUE, TRUE, &pageAllocation) == TRUE);

            // Point this entry to the base address of the next table
            pPageEntry->BaseAddress = pageAllocation.PA;
        }
        else
        {
            // We're initializing a PTE

            /*
             * Complete our identity mapping by setting the base address
             *  to the guest-physical base address (thereby creating a 1:1
             *  mapping from guest-physical addresses to underlying physical addresses)
             */
            pPageEntry->BaseAddress = PhysicalAddress;

            /*
             * [28.2.6.1] "Memory Type Used for Accessing EPT Paging Structures"
             * [A.1] "Basic VMX Information", "... software may map any of these regions
             *  or structures with the UC memory type. (This may be necessary for the MSEG
             *  header.) Doing so is discouraged unless necessary as it will cause the
             *  performance of software accesses to those structures to suffer."
             *
             * Generally, we'll always want our mappings to be of the write back (WB) memory type
             */
            pPageEntry->MemType = g_EPTP.MemType;

            // We've walked to the end of the paging tables using our target (guest) physical address,
            //  and created a 1:1 mapping with the host; so we're done here
            return TRUE;
        }
    }
    else if ( Altitude == EPT_ALTITUDE_PT )
    {
        // If the user supplied a physical address that already has a PTE
        //  in our EPT, we gloat in our past success
        return TRUE;
    }

    result = InsertEPTIdentityMapping(
        (Altitude - 1),
        pPageEntry[pageIndex].BaseAddress,
        PhysicalAddress
        );

    NT_ASSERT( result == TRUE );

    return TRUE;
}

BOOLEAN
InsertEPTEntry(
    _In_ CONST PVOID VirtualAddress
    )
{
    /*
     * So we need to insert the physical address of our target virtual address
     *  within the EPT. Things aren't that simple, though, as this concept holds true
     *  for every physical address which is utilized to access the physical address
     *  of our virtual address—think each of the page tables required to translate the
     *  target virtual address. For this reason, you can stop thinking about virtual
     *  addresses altogether, and think of the physical address of our virtual address
     *  as it's own virtual address. We call this a guest-physical address, and it must
     *  be indexable via our EPT.
     *
     * If you're like me, this concept may confuse you, as the reported max width (size in bytes)
     *  of your processor's physical addresses (seen with CPUID—see [3.3.1] "Intel® 64 Processors
     *  and Physical Address Space") may, and likely will be less than 48 bits. But this makes sense,
     *  as physical addresses typically are not required to index four, or any number of paging tables.
     *  Additionally, if we assume a flat 8GB physical address space, we only need 2^33 bits to index
     *  this region with 1-byte granularity; creating a max physical address width of 33 bits. For a
     *  flat 16GB physical address space, we require 34 bits; 35 bits for 32GB; and 36 bits for 64GB.
     *  You may wonder then, how we're able to index our EPT tables, and the answer seems fairly simple
     *  (in retrospect). Excluding the requirement of a PML5 (which no processors at this moment support),
     *  you will never have to index a PML4 table, as each PML4E indexes 512GB of memory. Each PDPTE, however,
     *  is capable of indexing 1GB of memory. So as long as we're able to index multiple PDPTEs, we can
     *  translate guest memory via our EPT tables. If we exclude the 16 bits of unused space at the top of a 48-bit
     *  virtual address, and the 9 bits required to index the PML4 table, that frees up 25 bits off of the physical
     *  address, leaving a minimum of 23 bits requied to index all 4 of our tables with 8-byte granularity;
     *  and then subsequently index a resulting 4KB page with 1-byte granularity. Now this is not the process
     *  of EPT translations, as the translations absolutely use 48-bits to index, and will in fact index
     *  a PML4 table (held in our EPTP). But separate PML4 tables are only used via the CR3 register to
     *  enforce virtual memory separation by operating systems (in the advent of segmentation being
     *  considered deprecated for this purpose); and as such, an unrestricted physical address space
     *  does not have the requirement for a PML4 table if it isn't indexing over 512GB of memory—the index
     *  into the PML4 will always be zero to index the first entry. So our EPT mechanism will use 48 bits,
     *  and address the PML4 table, but those topmost bits which way may not have set (per our max physical
     *  address width), will be then seen as zero, and index the first entry in our PML4 table.
     *
     * The only other thing to mention here is that 2MB entries require 21 bits (2^21 == 1024**2) to index at
     *  1-byte granularity, and paging tables which point to these LargePage data allocations must be aligned
     *  on a 2MB boundary. The same is true for 1GB LargePage mappings via PDPTEs.
     *
     * So anyway :), in order to index the target virtual address within our guest, we need to insert
     *  every physical address that will be individually indexed by the guest in order to
     *  obtain the final physical address for the target virtual address. These are the
     *  physical addresses of each of the 4 tables requierd to translate the virtual address
     *  in our guest (PML4, PDPT, PD, PT), as well as just the resulting physical address
     *  (after a full 4-level translation) because that final physical address might get
     *  cached in the TLB, and used as the sole index point of the desired data by the processor.
     */

    BOOLEAN result;

    PML4 pml4;

    // Outline our EPTP into a compatible PML4 for indexing with our general memory
    //  helper routines
    pml4.All = 0;
    pml4.PhysicalAddress = g_EPTP.BaseAddress;

    INDEX_POINTS targetIndexPoints = { 0 };

    // Gather the physical index points of our target virtual address (every
    //  physical address that must be present in the EPT for our target virtual address)
    NT_ASSERT(GetPhyscialIndexPoints(pml4, VirtualAddress, &targetIndexPoints) == TRUE);

    // Insert all of the required physical index points into the EPT

    // Identity map the PML4
    result = InsertEPTIdentityMapping(
        EPT_ALTITUDE_PML4,
        g_EPTP.BaseAddress,
        targetIndexPoints.PML4E.BaseAddress
        );

    NT_ASSERT( result == TRUE );

    // Identity map the PDPT
    result = InsertEPTIdentityMapping(
        EPT_ALTITUDE_PML4,
        g_EPTP.BaseAddress,
        targetIndexPoints.PDPTE.BaseAddress
        );

    NT_ASSERT( result == TRUE );

    // Identity map the PD
    result = InsertEPTIdentityMapping(
        EPT_ALTITUDE_PML4,
        g_EPTP.BaseAddress,
        targetIndexPoints.PDE.BaseAddress
        );

    NT_ASSERT( result == TRUE );

    // Identity map the PT
    result = InsertEPTIdentityMapping(
        EPT_ALTITUDE_PML4,
        g_EPTP.BaseAddress,
        targetIndexPoints.PTE.BaseAddress
        );

    NT_ASSERT( result == TRUE );

    // Identity map the Page
    result = InsertEPTIdentityMapping(
        EPT_ALTITUDE_PML4,
        g_EPTP.BaseAddress,
        targetIndexPoints.Page.BaseAddress
        );

    NT_ASSERT( result == TRUE );

    return TRUE;
}

/*
 * Recursive function used internally to index each page table within our EPT
 *  and deallocate all memory regions pointed to by individual entries.
 *
 * For no completeness (reason at all), this function supports 1GB and 2MB large pages.
 *
 * Parameters:
 *  `Altitude` here specifies what paging table index we're currently operating in,
 *    which is required to denote pages which map 4KB data allocations.
 *
 *  `PhysicalPageTable` holds the physical address of the base of the table we're
 *    currently operating in.
 */
BOOLEAN
_TeardownEPTTable(
    _In_opt_ CONST UINT8 Altitude,
    _In_opt_ CONST UINT64 PhysicalPageTable
    )
{
    // Note the usage of assertions.
    //  Because of the nature of this function, errors within are considered unrecoverable

    UINT32 iPageIdx;
    PHYSICAL_ADDRESS basePA;

    PVOID pVirtualPageTableBase, pDeallocationAddress;

    PEPT_GENERIC_PAGE pGenericTable;
    EPT_GENERIC_PAGE pageEntry;

    if ( (Altitude != 4 && PhysicalPageTable == 0)
        || (Altitude > 4 || Altitude < 1) )
    {
        return FALSE;
    }

    /*
     * If we pass an altitude of 4, and don't provide the physical base address
     *  of a paging table, then we will assume we're starting with the base address
     *  held in the EPTP (the PML4)
     */

    basePA.QuadPart = 0;
    basePA.QuadPart = (PhysicalPageTable != 0) ? PhysicalPageTable : g_EPTP.BaseAddress;

    pVirtualPageTableBase = MmGetVirtualForPhysical( basePA );

    NT_ASSERT( pVirtualPageTableBase != NULL );

    pGenericTable = (PEPT_GENERIC_PAGE)pVirtualPageTableBase;

    /*
     * Walk the page table we've been passed (512 entries at 8 bytes
     *  each (64-bit entries) adds up to PAGE_SIZE, which is the size
     *  of our internal page table allocations)
     */
    for ( iPageIdx = 0; iPageIdx < 512; iPageIdx++ )
    {
        pageEntry = pGenericTable[iPageIdx];

        // Check that we the have (the relevant) data to process this entry
        if ( pageEntry.BaseAddress != 0 )
        {
            // Ensure it points to something else; be that another paging table, or actual data
            if ( pageEntry.Present != FALSE )
            {
                /*
                 * Check if the data we're linking to is anoter page table, which won't have
                 *  the LargePage bit set (as that is reserved for pages mapping large data sets),
                 *  or be at altitude 1 (which is the level in which we index PTEs which,
                 *  if present, will always point to 4KB allocations)
                 */
                if ( pageEntry.LargePage == FALSE && Altitude != 1 )
                {
                    /*
                     * The page has a base address, and is present and points to something,
                     *  and that something is not actual data; so we are linking to another
                     *  page table, and that page table must be indexed individually
                     */
                    NT_ASSERT( TeardownEPT(pageEntry.BaseAddress, (Altitude - 1)) == TRUE );
                }

                // Regardless of if this entry is actual data, or a (now emptied)
                //  table indexing actual data, it needs to be deallocated

                basePA.QuadPart = 0;
                basePA.QuadPart = pageEntry.BaseAddress;

                pDeallocationAddress = MmGetVirtualForPhysical( basePA );

                NT_ASSERT(pDeallocationAddress != NULL);

                MmFreeContiguousMemory( pDeallocationAddress );
            }

            // If the page doesn't link to any data, or we have completed indexing
            //  and deallocating (or just deallocating), erase this entry from the table
            pGenericTable[iPageIdx].All = 0;
        }
    }

    return TRUE;
}

BOOLEAN
TeardownEPT()
{
    /*
     * Instruct the internal `_TeardownEPTTable` function to index 4 levels,
     *  and start with the base address held in the EPTP (PML4 table); effectively
     *  deallocating all of our internal EPT allocations (including the tables themselves).
     */
    return _TeardownEPTTable( EPT_ALTITUDE_PML4, 0 );
}
