#ifndef __GUEST_TARGET_H__
#define __GUEST_TARGET_H__

// A section used for the original code (TargetFn)
#pragma section("TARGET", read, execute)

__declspec(code_seg("TARGET"))
unsigned char
GuestTargetFn();

#endif // __GUEST_TARGET_H__
