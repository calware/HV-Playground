#include "EPT.h"

EPTP g_EPTP;

BOOLEAN
EptBuild()
{
    VMX_ADDRESS pml4Allocation;

    if ( utlAllocateVMXData(PAGE_SIZE, TRUE, TRUE, &pml4Allocation) == FALSE )
    {
        return FALSE;
    }

    g_EPTP.BaseAddress = (UINT64)pml4Allocation.PA >> PAGE_OFFSET_4KB;

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

BOOLEAN
EptIdentityMapSystem()
{
    UINT16 pdpteIdx, pdeIdx, gbCount;

    VMX_ADDRESS pdpt, pdt;

    PHYSICAL_ADDRESS eptpPML4PA;
    PVOID eptpPML4VA;

    PEPT_PML4E pPML4E;
    PEPT_PDPTE pPDPTE;
    PEPT_PDE pPDE;

    UINT64 pdePageAddress;

    MTRR_MEM_TYPE mtrrMemType;

    /*
     * Extract the EPTP's PML4 base address pointer (phyiscal address)
     *
     * Note: this operation is potentially unsafe (per future structural
     *  changes to the EPTP and other page table's high-level bits)
     */
    eptpPML4PA.QuadPart = g_EPTP.BaseAddress << PAGE_OFFSET_4KB;

    // Obtain the EPTP's PML4 base address pointer (virtual address)
    eptpPML4VA = MmGetVirtualForPhysical( eptpPML4PA );

    if ( eptpPML4VA == NULL )
    {
        return FALSE;
    }

    // Get the EPTP's PML4 table in a usable state
    pPML4E = (PEPT_PML4E)eptpPML4VA;

    // Get a rough estimate of the physical memory
    //  on the system (in gigabytes)
    gbCount = GetOptimalPhysMemMapSize();

    // Allocate a PDPT for the EPT (each PDPTE maps 1GB of memory)
    NT_ASSERT( utlAllocateVMXData(PAGE_SIZE, TRUE, TRUE, &pdpt) == TRUE );

    // Get the PDPT in a usable state
    pPDPTE = (PEPT_PDPTE)pdpt.VA;

    // Add this PDPT to our EPT's PML4 table in the form of a PML4E, and set
    //  the relevant property bits for the entry
    pPML4E[0].BaseAddress   = (UINT64)pdpt.PA >> PAGE_OFFSET_4KB;
    pPML4E[0].ReadAccess    =
    pPML4E[0].WriteAccess   =
    pPML4E[0].ExecuteAccess = TRUE;

    // We loop through every gigabyte of memory, represented by a PDPTE
    for ( pdpteIdx = 0; pdpteIdx < gbCount; pdpteIdx++ )
    {
        // Allocate a PDT for our current PDPTE
        NT_ASSERT( utlAllocateVMXData(PAGE_SIZE, TRUE, TRUE, &pdt) == TRUE );

        // Get a temporary PDT pointer
        pPDE = (PEPT_PDE)pdt.VA;

        // Add this PDT to our current PDPTE, setting the relevant property bits
        pPDPTE[pdpteIdx].BaseAddress    = (UINT64)pdt.PA >> PAGE_OFFSET_4KB;
        pPDPTE[pdpteIdx].ReadAccess     =
        pPDPTE[pdpteIdx].WriteAccess    =
        pPDPTE[pdpteIdx].ExecuteAccess  = TRUE;

        // Fill the PDT with PDEs corresponding to 2MB large pages, completing our 1GB mapping
        for ( pdeIdx = 0; pdeIdx < MAX_PDE_COUNT; pdeIdx++ )
        {
            // Calculate the page start address
            if (pdeIdx == 0)
            {
                /*
                 * If this is the first entry in our PDT, then we know
                 *  it starts at the beginning of a gigabyte boundary
                 *  based on our pdpteIndex
                 */

                pPDE[pdeIdx].Ref2MB.PageAddress = (pdpteIdx * _1GB) >> PAGE_OFFSET_2MB;
            }
            else
            {
                // If this is not the first entry, we can calculate the current 
                //  page start address by adding 2MB to the previous start address

                pdePageAddress = pPDE[pdeIdx - 1].Ref2MB.PageAddress << PAGE_OFFSET_2MB;

                pPDE[pdeIdx].Ref2MB.PageAddress = (pdePageAddress + _2MB) >> PAGE_OFFSET_2MB;
            }

            // Set the permission bits
            pPDE[pdeIdx].Ref2MB.ReadAccess      =
            pPDE[pdeIdx].Ref2MB.WriteAccess     =
            pPDE[pdeIdx].Ref2MB.ExecuteAccess   = TRUE;

            // Indicate a 2MB large page allocation
            pPDE[pdeIdx].Ref2MB.Ref2MBPage      = TRUE;

            // Obtain the memory type for this allocation
            //  (utilizing our MTRRs)
            MtrrGetMemType(
                pPDE[pdeIdx].Ref2MB.PageAddress << PAGE_OFFSET_2MB,
                _2MB,
                &mtrrMemType
                );

            // Set the corresponding MTRR memory type
            pPDE[pdeIdx].Ref2MB.MemType         = mtrrMemType;
        }
    }

    return TRUE;
}

BOOLEAN
EptGetPteForSystemAddress(
    _In_opt_ CONST PVOID SystemVA,
    _In_opt_ CONST UINT64 SystemPA,
    _Inout_ PEPT_PTE* CONST PTE
    )
{
    UINT8 tableLevel;
    UINT16 tableIndex;
    EPT_GENERIC_PAGE *pPageEntry, *pPageTable;

    GUEST_PA_LAYOUT guestPA;
    PHYSICAL_ADDRESS tableBasePA, targetPA;

    if ( (SystemVA == NULL && SystemPA == 0)
        || (SystemVA != NULL && SystemPA != 0) )
    {
        return FALSE;
    }

    if ( SystemVA != NULL )
    {
        targetPA = MmGetPhysicalAddress( SystemVA );

        if ( targetPA.QuadPart == 0 )
        {
            return FALSE;
        }
    }
    else
    {
        // SystemPA != NULL, per the checks by our above predicates
        targetPA.QuadPart = SystemPA;
    }
    
    guestPA.All = targetPA.QuadPart;

    // Gather our initial table address
    tableBasePA.QuadPart = TABLE_BASE_ADDRESS( g_EPTP.All );

    // Map and index each of our page tables
    //  Note: sloppy way to avoid another recursive function (or using
    //  GetPhysicalIndexPoints)

    for ( tableLevel = EPT_ALTITUDE_PML4, tableIndex = 0; tableLevel > 0; tableLevel-- )
    {
        // Grab the index for this page table
        switch ( tableLevel )
        {
            case EPT_ALTITUDE_PML4:
                tableIndex = (UINT16)guestPA.PML4Index;
                break;
            case EPT_ALTITUDE_PDPT:
                tableIndex = (UINT16)guestPA.PDPTIndex;
                break;
            case EPT_ALTITUDE_PD:
                tableIndex = (UINT16)guestPA.PDIndex;
                break;
            case EPT_ALTITUDE_PT:
                tableIndex = (UINT16)guestPA.PTIndex;
                break;
        }

        // Grab the virtual address for this page table
        pPageTable = (EPT_GENERIC_PAGE*)MmGetVirtualForPhysical( tableBasePA );

        if ( (PVOID)pPageTable == NULL )
        {
            return FALSE;
        }

        // Get the entry specified by our 'guest-physical address'
        pPageEntry = &pPageTable[tableIndex];

        // Ensure that we actually have an entry at this location
        if ( pPageEntry->All == 0 )
        {
            return FALSE;
        }

        // If pPageEntry points to a page table, then we need to do this again
        if (tableLevel != EPT_ALTITUDE_PT)
        {
            tableBasePA.QuadPart = TABLE_BASE_ADDRESS( pPageEntry->All );
        }
        else
        {
            // pPageEntry points to a PTE, and so we feed that back to the caller
            *PTE = (PEPT_PTE)pPageEntry;
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Recursive function used internally to index each page table within our EPT
 *  and deallocate all memory regions pointed to by individual entries.
 *
 * For completeness (reason at all), this function supports 1GB and 2MB large pages.
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
    basePA.QuadPart = (PhysicalPageTable != 0) ? PhysicalPageTable : TABLE_BASE_ADDRESS( g_EPTP.All );

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

        // Check if this entry actually points to something; be it a table or actual data
        if ( pageEntry.Present != FALSE )
        {
            /*
             * Check if the data we're linking to is another page table, which won't have
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

                // NT_ASSERT( TeardownEPT(pageEntry.BaseAddress, (Altitude - 1)) == TRUE );

                NT_ASSERT( _TeardownEPTTable(Altitude - 1, TABLE_BASE_ADDRESS(pageEntry.All)) == TRUE );

                basePA.QuadPart = 0;
                basePA.QuadPart = TABLE_BASE_ADDRESS( pageEntry.All );

                pDeallocationAddress = MmGetVirtualForPhysical( basePA );

                NT_ASSERT( pDeallocationAddress != NULL );

                MmFreeContiguousMemory( pDeallocationAddress );
            }

            pGenericTable[iPageIdx].All = 0;
        }
    }

    return TRUE;
}

BOOLEAN
EptTeardown()
{
    /*
     * Instruct the internal `_TeardownEPTTable` function to index 4 levels,
     *  and start with the base address held in the EPTP (PML4 table); effectively
     *  deallocating all of our internal EPT allocations (including the tables themselves).
     */
    return _TeardownEPTTable( EPT_ALTITUDE_PML4, 0 );
}
