#include "Seg.h"

/*
 * Notes for testing:
 *
 * Get the GDT base via `r gdtr`
 * Get the GDT limit via `r gdtl`
 * Get a full list of the segments in the GDT via `dg 0 <value in gdtl>`
 *  At this point you can match up that the `__segmentbase` and `ReadAR` functions
 *  perform the actions they claim to
 */

UINT64
__segmentbase(
	UINT16 selector
	)
{
	UINT64 baseAddress = 0;
	SYSTEM_TABLE_REGISTER gdtr = { 0 };

	PSEG_DESC pGDT;
	PSEG_DESC pGDTE;
	PSYS_SEG_DESC pGDTE64;
	SEG_SEL segmentSelector;

	__sgdt(&gdtr);

	pGDT = (PSEG_DESC)gdtr.Base;

	segmentSelector.All = selector;

	// Why lookup null entries if we don't have to
	//	(Windows 10 kernel LDT null, so this will filter out LDT entries creating a superfluous check on L54)
	if ( segmentSelector.Index == 0 )
	{
		return 0;
	}

	pGDTE = &pGDT[segmentSelector.Index];

	// Compose the 32-bit base address
	baseAddress = (pGDTE->Base2 << 24) | pGDTE->Base;
	baseAddress = ((pGDTE->Base2 << 24) | pGDTE->Base) & MAXUINT32;

	// Check if this entry is a system entry (double the size, and has more information)
	//	Note: others say to ignore LDT entries, or to only account for TSS entries; but I have found no reason
	//	as for why we should do that in either the SDM or Windows Internals, so I'm leaving this superflous check
	//	on the GDTE type here until further notice. If you're reading this comment, it means I was either right, or
	//	we have a potential bad error in the VMCS logic that will potentially cause bugs later :^)

	// If this is a system segment descriptor, and it's for either a TSS or LDT, we obtain the full 64-bit base address
	if ( pGDTE->DescType == DESCRIPTOR_TYPE_SYSTEM
		&& ((pGDTE->Type == SYS_SEG_DESC_TYPE_TSS_BUSY || pGDTE->Type == SYS_SEG_DESC_TYPE_TSS_AVAILABLE) // CHANGE HERE!
			|| pGDTE->Type == SYS_SEG_DESC_TYPE_LDT))
	{
		/*
		 *  In IA-32e mode, there are two types of system descriptors: system-segment descriptors and gate descriptors.[0]
		 *  System descriptors point to system segments, such as the LDT and TSS, while gate descriptors hold pointers to 
		 *  entry points in code segments, such as call, interrupt, and trap gates.
		 *
		 *  Both the LDT and TSS have the same expanded structural definition[1], and they are the only two system segment
		 *  descriptors we will ever encounter; as we only obtain the bases for CS/SS/DS/ES/FS/GS/LDTR/TR
		 *  (see setting of VMCS guest/host state areas in "Driver.c").
		 *
		 *  [0]: [3.5] "System Descriptor Types"
		 *	[1]: [7.2.3] "TSS Descriptor in 64-bit Mode"
		*/

		// Obtain the 64-bit system segment descriptor
		pGDTE64 = (PSYS_SEG_DESC)pGDTE;

		// Create the final 64-bit base address (if necessary)
		baseAddress |= ((UINT64)pGDTE64->Base3 << 32);
	}

	return baseAddress;
}

SEG_ACCESS_RIGHTS
ReadAR(
	UINT16 selector
	)
{
	// [3.2] "Using Segments"

	SEG_ACCESS_RIGHTS ar;

	/*
	 *  [Access rights] "Bit 16 indicates an unusable segment ... a segment
	 *  register is unusable if it has been loaded with a null selector." [0]
	 *
	 *  "The first entry of the GDT is not used by the processor. A segment
	 *  that points to this entry of the GDT (that is, a segment selector with
	 *  an index of 0 and the TI flag set to 0) is used as a 'null segment selector'"[1]
	 *
	 *  [0]: [24.4.1] "Guest Register State"
	 *  [1]: [3.4.2] "Segment Selectors"
	 */
	if ( selector == 0 )
	{
		/*
		 *  We get here because the kernel segments won't ever index via the LDT (always zero), which means the TI
		 *  bits will always be zero (indicating a GDT index), the RPL bits will always be zero, and the index being
		 *  clear signifies a null segment selector; which should be indicated via setting the unusable bit (16)
		 */
		ar.All = 0;
		ar.Unusable = TRUE;
		goto __ep;
	}

	// This will simply execute a `LAR` instruction with the provided selector
	ar = __readar(selector);

	ar.Unusable = FALSE;
	ar.Reserved0 = 0;
	ar.Reserved1 = 0;

__ep:
	return ar;

	// See also [26.3.2.2] "Loading Guest Segment Registers and Descriptor-Table Registers"
}
