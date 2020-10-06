#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <ntifs.h>

#include "VMCS.h"
#include "VMX.h"

#include "EPT.h"
#include "Guest.h"

#include "Log.h"
#include "Utils.h"



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



//
// Global definitions
//

static UINT8 g_BreakCount;

extern PCONTEXT g_preLaunchCtx;



//
// Structural definitions
//

typedef struct _EPT_SPLITTING_REGISTRATION
{
    PEPT_PTE TargetPte;
    EPT_PTE OriginalPte;
    UINT64 TargetBaseAddress;
    UINT64 SwapBaseAddress;
} EPT_SPLIT_REG;



//
// Function definitions
//
VOID
VMExitHandler(
    _In_ PCONTEXT CONST GuestContext
    );

#endif // __HANDLER_H__
