;
; Assembly intrinsic functions for EPT operations
;

.CODE

__invept PROC
    push 0
    push rdx
    invept rcx, xmmword ptr [rsp+8]
    pop rdx
    pop rdx
    ret
__invept ENDP

END
