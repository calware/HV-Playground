#include "EPT.h"

EPTP g_EPTP;
EPT_CACHED_PT g_pCachedPTList[EPT_CACHED_PT_COUNT];

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

VOID
EptInitializeCache()
{
    UINT8 cacheIdx;
    PEPT_CACHED_PT pCachedPT;

    VMX_ADDRESS pageTable;

    RtlSecureZeroMemory( (PVOID)&g_pCachedPTList, sizeof(g_pCachedPTList) );

    for ( cacheIdx = 0; cacheIdx < EPT_CACHED_PT_COUNT; cacheIdx++ )
    {
        pCachedPT = &g_pCachedPTList[cacheIdx];

        NT_ASSERT( utlAllocateVMXData(PAGE_SIZE, TRUE, TRUE, &pageTable) == TRUE );

        pCachedPT->PageTable = (PEPT_PTE)pageTable.VA;
        pCachedPT->PageTablePA = (UINT64)pageTable.PA;

        pCachedPT->Available = TRUE;
    }
}

BOOLEAN
EptInvalidateTlb(
    CONST INVEPT_TYPE Type
    )
{
    EPT_VPID_CAP eptCap;

    eptCap.All = __readmsr( IA32_VMX_EPT_VPID_CAP );

    // This plus prior preliminary checks prevents INVEPT from failing
    if ( Type == INVEPT_TYPE_SINGLE_CONTEXT && eptCap.INVEPTSingleContext == FALSE )
    {
        return FALSE;
    }
    else if ( Type == INVEPT_TYPE_GLOBAL_CONTEXT && eptCap.INVEPTAllContext == FALSE )
    {
        return FALSE;
    }

    __invept( Type, g_EPTP.All );

    return TRUE;
}

/*
 * Note: my testing machine isn't 1511+, so I can't use the
 *  newly-added KeGetEffectiveIrql here, instead of having
 *  a `UseCache` parameter. If your machine is updated, you
 *  should use KeGetEffectiveIrql instead.
 */
BOOLEAN
EptConvertLargePagePde(
    _In_ CONST BOOLEAN UseCache,
    _Inout_ PEPT_PDE CONST Pde,
    _Out_opt_ PEPT_PTE* CONST Pt
    )
{
    UINT8 cacheIdx;
    PEPT_CACHED_PT pCachedEntry;

    PEPT_PTE pageTable = NULL, tableEntry;
    UINT64 pageTablePa = 0;
    VMX_ADDRESS tableAllocation;

    UINT64 pde2MBRangeStart, ptePrevRangeStart;

    UINT16 pteIdx;

    EPT_PDE newPde = { 0 };

    // Ensure this PDE is both present, and marked as a large page allocation
    if ( Pde->Ref2MB.Ref2MBPage == FALSE || (Pde->All & EPT_PAGE_PRESENT_MASK) == 0 )
    {
        return FALSE;
    }

    pde2MBRangeStart = Pde->Ref2MB.PageAddress << PAGE_OFFSET_2MB;

    if (UseCache == TRUE)
    {
        // We're using the cache to obtain our page table allocation
        //  (typically seen in the VMM)
        for ( cacheIdx = 0; cacheIdx < EPT_CACHED_PT_COUNT; cacheIdx++ )
        {
            pCachedEntry = &g_pCachedPTList[cacheIdx];

            if ( pCachedEntry->Available == TRUE )
            {
                pageTable = pCachedEntry->PageTable;
                pageTablePa = pCachedEntry->PageTablePA;

                // This entry is no longer usable
                pCachedEntry->Available = FALSE;

                break;
            }
            else if ( cacheIdx == EPT_CACHED_PT_COUNT - 1 )
            {
                // We didn't have a free cache entry, so we fail here
                return FALSE;
            }
        }
    }
    else
    {
        // We're allocating a new page table
        //  (Note: unsafe assumption of current IRQL here)
        if ( utlAllocateVMXData(PAGE_SIZE, TRUE, TRUE, &tableAllocation) == FALSE )
        {
            return FALSE;
        }

        pageTable = (PEPT_PTE)tableAllocation.VA;
        pageTablePa = (UINT64)tableAllocation.PA;
    }
 
    // Fill this PT with the range encapsulated by our 2MB PDE
    for ( pteIdx = 0; pteIdx < MAX_PTE_COUNT; pteIdx++ )
    {
        tableEntry = &pageTable[pteIdx];

        // Set PTE permission bits
        tableEntry->ReadAccess      =
        tableEntry->WriteAccess     =
        tableEntry->ExecuteAccess   = TRUE;

        // Calculate the base address for this 4KB page
        if ( pteIdx == 0 )
        {
            /*
             * We're starting at the same address the PDE started at
             *  the only difference is that the user gets 12-bits
             *  instead of 21-bits to index this region (4KB at 1-byte granularity instead of 2MB)
             */

            tableEntry->BaseAddress = pde2MBRangeStart >> PAGE_OFFSET_4KB;
        }
        else
        {
            // Take the the base address of the previous 4KB PTE, convert it
            //  to an actual physical addrses, add 4KB, then write it back as PFN

            ptePrevRangeStart = pageTable[pteIdx - 1].BaseAddress << PAGE_OFFSET_4KB;

            tableEntry->BaseAddress = (ptePrevRangeStart + _4KB) >> PAGE_OFFSET_4KB;
        }
    }

    // Convert the large page PDE to point to our newly-created PT of 4KB PTEs

    //  Clone the permission bits
    newPde.RefPT.ReadAccess         = Pde->Ref2MB.ReadAccess;
    newPde.RefPT.WriteAccess        = Pde->Ref2MB.WriteAccess;
    newPde.RefPT.ExecuteAccess      = Pde->Ref2MB.ExecuteAccess;
    newPde.RefPT.UserExecuteAccess  = Pde->Ref2MB.UserExecuteAccess;

    //  Set the new page table base address (physical address!)
    newPde.RefPT.BaseAddress        = pageTablePa >> PAGE_OFFSET_4KB;

    //  Overwrite the existing PDE
    Pde->All = newPde.All;

    // Return to the user the newly-inserted page table base
    //  address if they asked for it
    if ( Pt != NULL )
    {
        *Pt = (PEPT_PTE)pageTable;
    }

    // Finally, we've changed a potentially cached paging entry,
    //  so we need to invalidate the TLB (unsafe assumption here)
    NT_ASSERT( EptInvalidateTlb(INVEPT_TYPE_GLOBAL_CONTEXT) == TRUE );

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
EptGetPte(
    _In_ CONST BOOLEAN VmmRequest,
    _In_ CONST PHYSICAL_ADDRESS TargetPa,
    _Inout_ PEPT_PTE* CONST Pte
    )
{
    UINT8 altitude;

    PVOID eptpPMl4Va = NULL;
    PHYSICAL_ADDRESS eptpPml4Pa;

    PEPT_PML4E pPml4;
    PEPT_PTE pPt;

    PEPT_GENERIC_PAGE pGenericTable, pGenericEntry;

    GUEST_PA_LAYOUT paLayout;
    UINT64 offset = 0;
    PHYSICAL_ADDRESS tableBasePa;
    PVOID tableBaseVa;
    
    // Obtain the EPT's PML4 physical base address
    eptpPml4Pa.QuadPart = g_EPTP.BaseAddress << PAGE_OFFSET_4KB;

    // Obtain the EPT's PML4 virtual base address
    eptpPMl4Va = MmGetVirtualForPhysical( eptpPml4Pa );
    NT_ASSERT( eptpPMl4Va != NULL );

    // Get this PML4 address in a usable form
    pPml4 = (PEPT_PML4E)eptpPMl4Va;

    // Convert the provided PHYSICAL_ADDRESS to obtain table indeces
    paLayout.All = TargetPa.QuadPart;

    // Set our initial physical table base
    tableBasePa.QuadPart = eptpPml4Pa.QuadPart;

    // Iterate our EPT tables and attempt to locate the desired PA
    for ( altitude = 4; altitude > 0; altitude-- )
    {
        // Need to figure out what the bit offset into the current
        //  table is
        switch (altitude)
        {
            case EPT_ALTITUDE_PML4:
                offset = paLayout.PML4Index;
                break;
            case EPT_ALTITUDE_PDPT:
                offset = paLayout.PDPTIndex;
                break;
            case EPT_ALTITUDE_PD:
                offset = paLayout.PDIndex;
                break;
            case EPT_ALTITUDE_PT:
                offset = paLayout.PTIndex;
                break;
        }

        // Get the virtual address of the current physical table base address
        tableBaseVa = MmGetVirtualForPhysical( tableBasePa );

        if ( tableBaseVa == NULL )
        {
            return FALSE;
        }

        pGenericTable = (PEPT_GENERIC_PAGE)tableBaseVa;

        // Obtain the target entry in this page table
        pGenericEntry = &pGenericTable[offset];

        // Possibility for a large-page PDE here which we may need to split
        if ( altitude == EPT_ALTITUDE_PD && pGenericEntry->LargePage == TRUE )
        {
            /*
             * Convert this 2MB large page entry into a standard PDE
             *  mapping a PT holding 512 4KB PTEs, and obtain
             *  the new PT's virtual base address
             */

            if ( EptConvertLargePagePde(VmmRequest, (PEPT_PDE)pGenericEntry, &pPt) == FALSE )
            {
                return FALSE;
            }

            // The above function returns to us the virtual base address
            //   of the newly-inserted page table, so all we have to do is index it

            *Pte = &pPt[paLayout.PTIndex];

            return TRUE;
        }
        else if ( altitude == EPT_ALTITUDE_PT )
        {
            *Pte = (PEPT_PTE)pGenericEntry;

            return TRUE;
        }

        // We're not a large page, and we're not a 4KB PTE, so we need to index further...

        //  Get the next table base address and continue the loop
        tableBasePa.QuadPart = pGenericEntry->BaseAddress << PAGE_OFFSET_4KB;
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
    UINT8 cacheIdx;
    PEPT_CACHED_PT pCacheEntry;

    /*
     * Teardown the unused cache entries here (the private
     *  _TeardownEPTTable function, utilized below, will
     *  take care of used cache entries)
     */
    for ( cacheIdx = 0; cacheIdx < EPT_CACHED_PT_COUNT; cacheIdx++ )
    {
        pCacheEntry = &g_pCachedPTList[cacheIdx];

        if (pCacheEntry->Available == TRUE)
        {
            MmFreeContiguousMemory( (PVOID)pCacheEntry->PageTable );

            pCacheEntry->Available = FALSE;
        }
    }

    /*
     * Instruct the internal `_TeardownEPTTable` function to index 4 levels,
     *  and start with the base address held in the EPTP (PML4 table); effectively
     *  deallocating all of our internal EPT allocations (including the tables themselves).
     */
    return _TeardownEPTTable( EPT_ALTITUDE_PML4, 0 );
}
