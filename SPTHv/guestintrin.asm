
; This file is responsible for providing guest assembly intrinsic functions
; (__hlt), as well as a test function written in assembly, to allow users
; to test more granular operations in our guest execution environment.
;
; See the parent header, "Guest.h"

; =================================================

.CODE HLTASM

__hlt PROC
    hlt
    ret
__hlt ENDP

HLTASM ENDS

; =================================================

.CODE

RawGuestEntry PROC
    mov rax, 2
    mov rcx, 1
    hlt
    sub rcx, rax
    mov rax, 1
    mov rcx, 0
    adc rax, rcx
    hlt
    xor rax, rax
    xor rax, rcx
    mov rdx, 10
    mov rbx, 1337h
    hlt
RawGuestEntry ENDP

END
