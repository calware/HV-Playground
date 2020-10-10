#ifndef __GUEST_H__
#define __GUEST_H__

#include <wdm.h>

#include <intrin.h>



//
// External definitions (see "./guestintrin.asm")
//

extern void __hlt();

extern void RawGuestEntry();



//
// Source definitions (see "./Guest.c")
//
VOID
GuestEntry();

#endif // __GUEST_H__
