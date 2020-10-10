; 
; This is the assembly source file to primarily support operations concerning
;  segmentation that are not already supported by the Microsoft
;  provided intrinsic functions (see "intrin.h")
; 

.code

; 
; Read the CS selector
;
__readcs PROC
	mov ax, cs
	ret
__readcs ENDP


;
; Read the SS selector
;
__readss PROC
	mov ax, ss
	ret
__readss ENDP


;
; Read the DS selector
;
__readds PROC
	mov ax, ds
	ret
__readds ENDP


;
; Read the ES selector
;
__reades PROC
	mov ax, es
	ret
__reades ENDP


;
; Read the FS selector
;
__readfs PROC
	mov ax, fs
	ret
__readfs ENDP


;
; Read the GS selector
;
__readgs PROC
	mov ax, gs
	ret
__readgs ENDP


;
; Read the LDTR selector
;
__readldtr PROC
	sldt ax
	ret
__readldtr ENDP


;
; Read the TR selector
;
__readtr PROC
	str ax
	ret
__readtr ENDP


;
; Read the GDTR
;
__sgdt PROC
	sgdt [rcx]
	ret
__sgdt ENDP


;
; Get the access rights to a segment
;
__readar PROC
	xor rax, rax
	lar rax, rcx
	jnz _ep			; no zero flag set indicates an error
	shr rax, 8		; ignore the first byte (see [2.A] "LAR—Load Access Rights Byte", pg.1139)
_ep:
	ret
__readar ENDP


;
; Store the GDTR
;
__lgdt PROC
	lgdt fword ptr [rcx]
	ret
__lgdt ENDP


;
; Read the stack pointer
;
__readsp PROC
    mov rax, rsp
    ret
__readsp ENDP

end
