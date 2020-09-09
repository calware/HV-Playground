#include "Guest.h"

VOID
GuestEntry()
{
    /*
     *  Note: this function is used to provide both the user and the guest environment
     *  with a C function (rather than an assembly function—see "./guestintrin.asm") to allow for
     *  the facilitation of more complicated guest and VMM operations
     */

    unsigned char a = 0x88;

    __hlt();

    a = 0x99;

    __hlt();
}
