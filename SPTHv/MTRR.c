#include "MTRR.h"

BOOLEAN
CheckMTTRSupport()
{
    CPUID_INFO cpuidInfo = { 0 };

    MTRR_DEFS mtrrDefs;

    __cpuid( (int*)&cpuidInfo, CPUID_FEATURE_INFORMATION );

    // Check if the MTRRs are enabled
    //  [11.11.1] "MTRR Feature Identification"
    if ( (cpuidInfo.EDX & CPUID_FEATURE_MTRR_ENABLED) == 0 )
    {
        return FALSE;
    }

    // Check the MTRR default information MSR
    //  [11.11.2.1] "IA32_MTRR_DEF_TYPE MSR"
    mtrrDefs.All = __readmsr( IA32_MTRR_DEF_TYPE );

    return (BOOLEAN)mtrrDefs.MTRRSupport;
}



VOID
MtrrGetMemType(
    _In_ CONST UINT64 RangeStart,
    _In_ CONST SIZE_T RangeSize,
    _Inout_ MTRR_MEM_TYPE* CONST MemType
    )
{
    MTRR_CAP mtrrCap;
    UINT8 vIdx, fIdx, fSubIdx;

    FIXED_RANGE_ENTRY fixedEntry;
    UINT8 shiftSize;

    UINT64 fixedMask, varMask;

    MTRR_DEFS mtrrDefs;

    MTRR_PHYS_BASE physBase;
    MTRR_PHYS_MASK physMask;

    ULONG bitIdx;
    UINT64 rangeBase, rangeLimit,
        searchBase, searchLimit,
        subBase, subLimit;

    MTRR_MEM_TYPE tempMemType;
    MTRR_MEM_TYPE regionMemRecord = MTRR_MEM_UNKNOWN;

    UINT64 phyAddrMask = CreateMaxPhyAddrMask();

    // Set the bounds of our memory range search below
    searchBase = RangeStart;
    searchLimit = searchBase + (RangeSize - 1);

    // Get the MTRR default memory type
    mtrrDefs.All = __readmsr( IA32_MTRR_DEF_TYPE );

    // Get the MTRR capability information so that we know what to index
    mtrrCap.All = __readmsr( IA32_MTRRCAP );

    // Check if the fixed-range MTRRs are enabled
    if ( mtrrCap.FixedSupport == TRUE )
    {
        // Check if the the search range falls within the bounds
        //  of our fixed-range MTRRs
        if ( searchBase <= FIXED_MTRR_RANGE_HIGH )
        {
            // Scan across our 11 fixed-range MTRRs for memory types
            for ( fIdx = 0; fIdx < MTRR_FIXED_RANGE_REG_COUNT; fIdx++ )
            {
                fixedEntry = g_FixedRanges[fIdx];

                // Is any part of the provided search range within 
                //  any of this fixed-range register's sub-ranges
                if ( searchBase <= fixedEntry.EndAddress && searchLimit >= fixedEntry.StartAddress )
                {
                    // Iterate through each of the sub ranges
                    for ( fSubIdx = 0; fSubIdx < MTRR_FIXED_RANGE_SUB_SET_COUNT; fSubIdx++ )
                    {
                        // Calculate the base and limit of this sub range
                        subBase = fixedEntry.StartAddress + (fixedEntry.SubRangeSize * fSubIdx);
                        //  Account for zero-indexing
                        subLimit = subBase + (fixedEntry.SubRangeSize - 1);

                        // Check if the provided search range falls within this sub-range
                        if ( searchBase <= subLimit && searchLimit >= searchBase )
                        {
                            /*
                             * Each of these 11 range registers has 8 sub ranges.
                             *  The memory type for these corresponding 8 sub ranges
                             *  is stored in 8-bit segments in the corresponding range
                             *  register's mask MSR
                             */

                            // Get the sub region memory types�encoded in the mask value
                            fixedMask = __readmsr( fixedEntry.MaskMSR );

                            // Get the shift size to access the memory type for this sub-region
                            shiftSize = MTRR_FIXED_RANGE_SUB_SET_COUNT * fSubIdx;

                            // Take the memory type from this sub range into account for
                            //   the overall search range
                            tempMemType = (MTRR_MEM_TYPE)( (fixedMask >> shiftSize) & 0xFF );

                            // See [0] below ...
                            if ( tempMemType < regionMemRecord )
                            {
                                //  Maybe a potential problem here in setting a WP memory type
                                //  for ranges that require write privileges?

                                regionMemRecord = tempMemType;

                                if ( regionMemRecord == MTRR_MEM_UC )
                                {
                                    goto __set_and_ret;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Loop through each of the variable range MTRRs
    for ( vIdx = 0; vIdx < mtrrCap.VariableCount; vIdx++, bitIdx = 0 )
    {
        // Get the current variable MTRR PhysBase and PhysMask
        physBase.All = GET_VAR_MTRR_PHYSBASE( vIdx );
        physMask.All = GET_VAR_MTRR_PHYSMASK( vIdx );

        if ( physMask.Valid == FALSE )
            continue;

        // Get the MTRR range base address
        rangeBase = physBase.All & phyAddrMask;

        // Get the MTRR mask
        varMask = physMask.All & phyAddrMask;

        // Calculate the MTRR range length
        //  (found by taking the first LSB in the mask as a value, seen below before we subtract one)
        _BitScanForward64( &bitIdx, varMask );

        // We subtract one here from the range length to account
        //  for the memory being zero-indexed
        rangeLimit = rangeBase + ( ((UINT64)1 << bitIdx) - 1 );

        // Check if any of the memory within our search bounds lies inside of this memory range
        if ( searchBase <= rangeLimit && searchLimit >= rangeBase )
        {
            /*
             * [0]: Here, we generally want to save the memory type associated with this
             *  region, as a region specifying a memory type was contained within
             *  the bounds of our search. The problem is, if this MTRR range specifies
             *  memory that conflicts with past memory types (if we lie on a boundary),
             *  we can't always set a new memory type. For instance, if the past MTRR range
             *  yielded a UC memory type for some portion of our search region, and the next
             *  region specifies a WB memory type for another portion, we shouldn't use WB
             *  over UC for the whole. *I believe* this concept holds true in using WB over
             *  WC/WT/WP as well. It's in that believe that I have decided to scan for what
             *  I believe to be more restrictive memory types here, and then default to
             *  the most restrictive in the return.
             *
             * In all likelihood, this is unnecessary, and will only really result in
             *  the usual check for a portion of our search region lying anywhere in
             *  UC memory; and subsequently defaulting to that memory type.
             *
             * See Vol.3A, Table 11-2, "Memory Types and Their Properties"
             */
            if ( physBase.MemType < regionMemRecord )
            {
                regionMemRecord = (MTRR_MEM_TYPE)physBase.MemType;

                // Check if this new region memory type is the most restrictive
                //  (which would mean that there's no reason to continue scanning)
                if ( regionMemRecord == MTRR_MEM_UC )
                {
                    goto __set_and_ret;
                }
            }
        }
    }

    // If we couldn't find the provided range in any of the MTRRs,
    //  then we just use the default memory type for this range
    if ( regionMemRecord == MTRR_MEM_UNKNOWN )
    {
        /*
         * Default memory type for regions not within the bounds of our MTRRs.
         * I believe this will always be UC, which is why 
         *  (See [11.11.2.1] "IA32_MTRR_DEF_TYPE MSR")
         */
        regionMemRecord = (MTRR_MEM_TYPE)mtrrDefs.MemType;
    }

__set_and_ret:

    // Return the memory type located by this search
    *MemType = regionMemRecord;
}
