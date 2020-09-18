#include "Memory.h"

BOOLEAN
GetPhyscialIndexPoints(
    _In_ CONST PML4 PhysicalPML4,
    _In_ CONST PVOID VirtualAddress,
    _Out_ PINDEX_POINTS CONST IndexPoints
    )
{
    PML4E* pPML4E;
    PDPTE* pPDPTE;
    PDE* pPDE;
    PTE* pPTE;

    VA_LAYOUT VAParts;
    PHYSICAL_ADDRESS targetPA, expectedPA;

    VAParts.All = (UINT64)VirtualAddress;
    targetPA.QuadPart = 0;

    if ( PhysicalPML4.All == 0 || VirtualAddress == 0 || IndexPoints == NULL )
    {
        return FALSE;
    }

    expectedPA = MmGetPhysicalAddress( VirtualAddress );
    if ( expectedPA.QuadPart == 0 )
    {
        return FALSE;
    }

    // Store the base address of our PML4; which must be indirectly indexable via our EPT tables
    IndexPoints->PML4E.BaseAddress = 
                 targetPA.QuadPart = TABLE_BASE_ADDRESS( PhysicalPML4.All );

    // Further craft and store the address of our PML4E
    IndexPoints->PML4E.Fields.All = 
                targetPA.QuadPart += VAParts.PML4Index * sizeof(UINT64);

    // 'Map' the PML4E
    pPML4E = (PML4E*)MmGetVirtualForPhysical( targetPA );

    if ( (PVOID)pPML4E == NULL || pPML4E->PhysicalAddress == 0 )
    {
        return FALSE;
    }

    // Store the base address of our PDPT
    IndexPoints->PDPTE.BaseAddress =
                 targetPA.QuadPart = TABLE_BASE_ADDRESS( pPML4E->All );

    // Craft and store the address of our PDPTE
    IndexPoints->PDPTE.Fields.All =
                targetPA.QuadPart += VAParts.PDPTIndex * sizeof(UINT64);

    // 'Map' the PDPTE
    pPDPTE = (PDPTE*)MmGetVirtualForPhysical( targetPA );

    if ( (PVOID)pPDPTE == NULL || pPDPTE->PhysicalAddress == 0
        || LARGE_PAGE_MAPPING(pPDPTE->All) )
    {
        return FALSE;
    }

    // Store the base address of our PDT
    IndexPoints->PDE.BaseAddress =
               targetPA.QuadPart = TABLE_BASE_ADDRESS( pPDPTE->All );

    // Craft and store the address of our PDE
    IndexPoints->PDE.Fields.All =
              targetPA.QuadPart += VAParts.PDIndex * sizeof(UINT64);

    // 'Map' the PDE
    pPDE = (PDE*)MmGetVirtualForPhysical( targetPA );

    if ( (PVOID)pPDE == NULL || pPDE->PhysicalAddress == 0
        || LARGE_PAGE_MAPPING(pPDE->All) )
    {
        return FALSE;
    }

    // Store the base address of our PT
    IndexPoints->PTE.BaseAddress =
               targetPA.QuadPart = TABLE_BASE_ADDRESS(pPDE->All);

    // Craft and store the address of our PTE
    IndexPoints->PTE.Fields.All =
              targetPA.QuadPart += VAParts.PTIndex * sizeof(UINT64);

    // 'Map' the PTE
    pPTE = (PTE*)MmGetVirtualForPhysical( targetPA );

    if ( (PVOID)pPTE == NULL || pPTE->PhysicalAddress == 0 )
    {
        return FALSE;
    }

    // Store the PTE's pointer to our actual data, as well as simplified
    //  pointer to our actual data
    IndexPoints->Page.Fields.All = pPTE->All;
    IndexPoints->Page.BaseAddress = TABLE_BASE_ADDRESS(pPTE->All);

    // Check that we have successfully walked to our target page
    if ( IndexPoints->Page.BaseAddress != TABLE_BASE_ADDRESS(expectedPA.QuadPart) )
    {
        return FALSE;
    }

    return TRUE;
}
