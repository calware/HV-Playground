#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <ntifs.h>
#include <intrin.h>

#include "CPU.h"
#include "MSR.h"
#include "VMX.h"
#include "VMCS.h"
#include "Seg.h"

#include "Utils.h"

DRIVER_INITIALIZE DriverEntry;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#endif // ALLOC_PRAGMA

//
// External definitions
//

NTSYSAPI
VOID NTAPI
RtlRestoreContext (
	PCONTEXT ContextRecord,
	PEXCEPTION_RECORD ExceptionRecord
	);

extern void GuestEntry();



//
// Structural definitions
//

typedef struct _LP_INFO
{
	VMX_ADDRESS VMCode;
	VMX_ADDRESS VMStack;
	VMX_ADDRESS HostCode;
	VMX_ADDRESS HostStack;
	VMX_ADDRESS VMXONRegion;
	VMX_ADDRESS VMCS;
	VMX_ADDRESS MSRBitmap;
} LP_INFO, *PLP_INFO;



//
// Globals
//

static LP_INFO g_LPInfo;

static CONTEXT g_preLaunchCtx;

static SYSTEM_TABLE_REGISTER g_GDTR, g_IDTR;

static CR0 g_CR0;

static CR4 g_CR4;


//
// Function definitions
//

VOID
VMExitHandler();

NTSTATUS
DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
	);


#endif // __DRIVER_H__
