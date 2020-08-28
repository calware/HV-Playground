#include "Utils.h"

BOOLEAN
utlAllocateVMXData (
	_In_ CONST SIZE_T Length,
	_In_ CONST BOOLEAN Contiguous,
	_In_opt_ CONST BOOLEAN PhysicalAddress,
	_Inout_ CONST PVMX_ADDRESS AllocationAddress
	)
{
	BOOLEAN bResult = FALSE;

	PHYSICAL_ADDRESS HighBound;
	HighBound.QuadPart = ~0;

	AllocationAddress->VA = (
		(Contiguous == TRUE)
			? MmAllocateContiguousMemory( Length, HighBound )
			: ExAllocatePoolWithTag( NonPagedPool, Length, SPTHV_POOL_TAG )
		);

	if ( AllocationAddress->VA == NULL )
	{
		goto __ep;
	}

	if ( PhysicalAddress == TRUE )
	{
		AllocationAddress->PA = (PVOID)(MmGetPhysicalAddress( AllocationAddress->VA )).QuadPart;
		if ( AllocationAddress->PA == NULL )
		{
			goto __ep;
		}
	}

	RtlSecureZeroMemory( AllocationAddress->VA, Length );

	bResult = TRUE;

__ep:
	if ( bResult == FALSE && AllocationAddress->VA != NULL )
	{
		(Contiguous == TRUE)
			? MmFreeContiguousMemory( AllocationAddress->VA )
			: ExFreePoolWithTag( AllocationAddress->VA, SPTHV_POOL_TAG );

		AllocationAddress->VA = NULL;
	}

	return bResult;
}

VOID
utlFreeVMXData(
	_Inout_ CONST PVMX_ADDRESS Allocation,
	_In_ CONST BOOLEAN Contiguous
	)
{
	(Contiguous == TRUE)
		? MmFreeContiguousMemory( Allocation->VA )
		: ExFreePoolWithTag( Allocation->VA, SPTHV_POOL_TAG );

	Allocation->VA = NULL;
	Allocation->PA = NULL;
}

void
utlGetNextInstrAddr(
	_Inout_ UINT64* CONST pAddr
	)
{
	*pAddr = (UINT64)_ReturnAddress();
}
