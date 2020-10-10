#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <ntifs.h>

#include "VMCS.h"
#include "VMX.h"
#include "CPU.h"

#include "Log.h"



//
// External definitions
//

// See "rawhandler.asm"
extern VOID
RawHandler();

// Custom internal implementation of RtlRestoreContext (see "rawhandler.asm")
extern
VOID DECLSPEC_NORETURN
RestoreContext(
    _In_ PCONTEXT ContextRecord
    );

// Function that will be used to switch out the CurrentThread's saved stack base & bound combination
extern
VOID
SwitchPrcbStackEntries(
    _In_ UINT64 StackBase,
    _In_ UINT64 StackLimit
    );

// Function used to get the CurrentThread's stack base and bounds
extern
VOID
GetPrcbStackEntries(
    _Inout_ PUINT64 StackBase,
    _Inout_ PUINT64 StackLimit
    );



//
// Global definitions
//

static UINT8 g_BreakCount;

extern PCONTEXT g_preLaunchCtx;

extern UINT64 g_GuestStackBase;
extern UINT64 g_GuestStackLimit;
extern UINT64 g_VMMStackBase;
extern UINT64 g_VMMStackLimit;
extern UINT64 g_InitialStackBase;
extern UINT64 g_InitialStackLimit;



//
// Function definitions
//
VOID
VMExitHandler(
    _In_ PCONTEXT CONST GuestContext
    );

#endif // __HANDLER_H__
