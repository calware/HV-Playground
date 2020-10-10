#include "Guest.h"

VOID
GuestEntry()
{
    /*
     *  Note: this function is used to provide both the user and the guest environment
     *  with a C function (rather than an assembly function—see "./guestintrin.asm") to allow for
     *  the facilitation of more complicated guest and VMM operations
     */

    UINT8 status;

    __hlt();

    __try
    {
        __vmx_vmlaunch();

        status = 0xAA;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = 0xBB;
    }

    __hlt();
}
