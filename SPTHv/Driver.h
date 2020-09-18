#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <ntifs.h>
#include <intrin.h>

#include "CPU.h"
#include "MSR.h"
#include "VMX.h"
#include "VMCS.h"
#include "Seg.h"

#include "Handler.h"
#include "Guest.h"

#include "EPT.h"

#include "Utils.h"

DRIVER_INITIALIZE DriverEntry;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#endif // ALLOC_PRAGMA



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

static SYSTEM_TABLE_REGISTER g_GDTR, g_IDTR;

static CR0 g_CR0;

static CR4 g_CR4;

extern PCONTEXT g_preLaunchCtx;



//
// Function definitions
//

NTSTATUS
DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
	);


#endif // __DRIVER_H__
