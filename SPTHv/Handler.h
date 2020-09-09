#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <ntifs.h>

#include "VMCS.h"
#include "VMX.h"

#include "Log.h"

//
// External definitions
//

// NTSYSAPI
// VOID NTAPI
// RtlRestoreContext (
// 	PCONTEXT ContextRecord,
// 	PEXCEPTION_RECORD ExceptionRecord
// 	);

// See "rawhandler.asm"
extern VOID
RawHandler();

// Custom internal implementation of RtlRestoreContext (see "rawhandler.asm")
extern
VOID DECLSPEC_NORETURN
RestoreContext(
    _In_ PCONTEXT ContextRecord
    );



//
// Global definitions
//

static UINT8 g_BreakCount;

extern PCONTEXT g_preLaunchCtx;



//
// Function definitions
//
VOID
VMExitHandler(
    _In_ PCONTEXT CONST GuestContext
    );

#endif // __HANDLER_H__
