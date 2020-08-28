#ifndef __UTILS_H__
#define __UTILS_H__

#include <ntddk.h>

#define SPTHV_POOL_TAG 'HTPS'

typedef struct _VMX_ADDRESS
{
	PVOID VA;
	PVOID PA;
} VMX_ADDRESS, *PVMX_ADDRESS;

BOOLEAN
utlAllocateVMXData (
	_In_ CONST SIZE_T Length,
	_In_ CONST BOOLEAN Contiguous,
	_In_opt_ CONST BOOLEAN PhysicalAddress,
	_Inout_ CONST PVMX_ADDRESS AllocationAddress
	);

VOID
utlFreeVMXData(
	_Inout_ CONST PVMX_ADDRESS Allocation,
	_In_ CONST BOOLEAN Contiguous
	);

void
utlGetNextInstrAddr (
	_Inout_ UINT64* CONST pAddr
	);

#endif // __UTILS_H__
