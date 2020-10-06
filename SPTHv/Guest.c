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

    UINT64 fnCodeBytes;

    __hlt();

    // Get the first 8-bytes at GuestTargetFn
    fnCodeBytes = *(UINT64*)GuestTargetFn;

    /*
     * Check that the code bytes at the target function
     *  are as they should be (ensure remapping from
     *  GuestTargetFn to GuestHookFn didn't occur)
     *
     * GuestTargetFn()
     *   b0aa     mov al, 0AAh
     *   c3       ret
     */
    if ( fnCodeBytes == 0xC3AAB0 )
    {
        // If they're the same, call our GuestTargetFn
        returnValue = GuestTargetFn();
    }
    else
    {
        // If the read didn't match what was expected,
        //  then simply return 0xCC on the stack
        returnValue = 0xCC;
    }

    // Signal to the VMM that we're done with our
    //  guest code execution
    __hlt();
}
