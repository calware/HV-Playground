#include "Guest.h"

__declspec(code_seg("TARGET"))
UINT8
GuestTargetFn()
{
    return 0xAA;
}

__declspec(code_seg("HOOK"))
UINT8
GuestHookFn()
{
    return 0xBB;
}

__declspec(code_seg("GUEST"))
VOID
GuestEntry()
{
    /*
     *  Note: this function is used to provide both the user and the guest environment
     *  with a C function (rather than an assembly function—see "./guestintrin.asm") to allow for
     *  the facilitation of more complicated guest and VMM operations
     */

    UINT8 returnValue = 0;

    __hlt();

    returnValue = GuestTargetFn();

    __hlt(); // Check that 0xBB is present on the stack, instead of 0xAA
}
