#ifndef __GUEST_H__
#define __GUEST_H__

#include <ntddk.h>

#include "Utils.h"

/*
 * Here we're directing the creation of three new code sections.
 *  Each section will hold either the guest entry point, the hook function,
 *  or the original target function, which is to be called by the guest
 *  entry point. Additionally, another code section called "HLTASM"
 *  is created within `guestintrin.asm` to support calls to the HLT
 *  instruction, which is effectively a breakpoint for our guest code.
 *
 * The reason we do this is to explicitly direct the windows PE loader
 *  to outline these functions directly into their own sections, thereby
 *  creating individual allocations for each of these functions (the only
 *  ones provided to their respective sections) which will be of at least PAGE_SIZE.
 *  This allows us to have function start addresses perfectly aligned to
 *  page boundaries, and moreover provides reasonable assurance that our
 *  functions won't gap page boundaries (and spill over into another page—
 *  which we would then be required to be indexable via our EPT tables)
 */

// A section used for the guest code (GuestEntry)
#pragma section("GUEST", read, execute)

// A section used for the original code (TargetFn)
#pragma section("TARGET", read, execute)

// A section used for the redirection code (HookFn)
#pragma section("HOOK", read, execute)



//
// External definitions (see "./guestintrin.asm")
//

// Note: this function is also now oultined into it's own
//  section called "HLTASM", and falls on a page boundary
extern void __hlt();

extern void RawGuestEntry();



//
// Source definitions (see "./Guest.c")
//

__declspec(code_seg("GUEST"))
VOID
GuestEntry();

__declspec(code_seg("TARGET"))
UINT8
GuestTargetFn();

__declspec(code_seg("HOOK"))
UINT8
GuestHookFn();

#endif // __GUEST_H__
