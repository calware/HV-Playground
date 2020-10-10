
; This file is responsible for saving guest state between exits and providing
;  our C exit handler (in "Driver.c") with code to continue the guest.
;
; See the parent header "Handler.h"

include ksamd64.inc

.DATA

EXTERN VMExitHandler: PROC
EXTERN RtlCaptureContext: PROC

VMCS_HOST_RSP       EQU   6C14h
VMCS_GUEST_RSP      EQU   681Ch
VMCS_GUEST_RIP      EQU   681Eh
VMCS_GUEST_RFLAGS   EQU   6820h

.CODE

RestoreContext PROC
    ;   Internal function to restore processor context based on a provided ContextRecord
    ;   structure.
    ;
    ;   Note: this function can be observed in both SimpleVisor and HyperBone; both of
    ;   which are direct copy-pastes of the original RtlRestoreContext function, just
    ;   without support for an ExceptionRecord and missing a call to KeCheckStackAndTargetAddress
    ;   (Win10 15063+), which causes a KERNEL_SECURITY_CHECK_FAILURE bugcheck.
    ;
    ;   See:
    ;     SimpleVisor   https://github.com/ionescu007/SimpleVisor/blob/master/nt/shvosx64.asm#L47
    ;     HyperBone     https://github.com/DarthTon/HyperBone/blob/master/src/Arch/Intel/VMXa.asm#L29

    ; Restore our XMM registers
    movaps xmm0, xmmword ptr [rcx+1A0h]
    movaps xmm1, xmmword ptr [rcx+1B0h]
    movaps xmm2, xmmword ptr [rcx+1C0h]
    movaps xmm3, xmmword ptr [rcx+1D0h]
    movaps xmm4, xmmword ptr [rcx+1E0h]
    movaps xmm5, xmmword ptr [rcx+1F0h]
    movaps xmm6, xmmword ptr [rcx+200h]
    movaps xmm7, xmmword ptr [rcx+210h]
    movaps xmm8, xmmword ptr [rcx+220h]
    movaps xmm9, xmmword ptr [rcx+230h]
    movaps xmm10, xmmword ptr [rcx+240h]
    movaps xmm11, xmmword ptr [rcx+250h]
    movaps xmm12, xmmword ptr [rcx+260h]
    movaps xmm13, xmmword ptr [rcx+270h]
    movaps xmm14, xmmword ptr [rcx+280h]
    movaps xmm15, xmmword ptr [rcx+290h]

    ; Restore SSE control/status register
    ldmxcsr dword ptr [rcx+34h]

    ; Restore GPRs and our BP
    mov rax, [rcx+78h]
    mov rdx, [rcx+88h]
    mov r8, [rcx+0B8h]
    mov r9, [rcx+0C0h]
    mov r10, [rcx+0C8h]
    mov r11, [rcx+0D0h]
    mov rbx, [rcx+90h]
    mov rsi, [rcx+0A8h]
    mov rdi, [rcx+0B0h]
    mov rbp, [rcx+0A0h]
    mov r12, [rcx+0D8h]
    mov r13, [rcx+0E0h]
    mov r14, [rcx+0E8h]
    mov r15, [rcx+0F0h]

    cli                     ; Disable interrupts

    push [rcx+044h]         ; Push FLAGS
    popfq                   ; Restore our FLAGS

    mov rsp, [rcx+98h]      ; Restore our saved stack pointer
    push [rcx+0F8h]         ; Push our instruction pointer onto the stack
    mov rcx, [rcx+80h]      ; Restore our initial RCX value
    ret                     ; Restore our instruction pointer
RestoreContext ENDP

_VMREAD MACRO
    ; Macro function used to simplify VMREAD operations in our RawHandler function.
    ; Also used to supply additional error information upon unsuccessful VMREAD operations.
    ;
    ; Thanks to the microsoft intrinsic functions (although this function has heavy edits)

    xor rax, rax
    xor r8, r8
    push rcx                ; Preserve our RCX value
    vmread r8, rdx          ; RDX holds our VMCS encoding, r8 may end up holding our VMCS field value
    sete al                 ; AL == ZF
    setb cl                 ; CL == CF
    adc al, cl              ; AL = AL + CL + CF
    pop rcx                 ; Restore our RCX value
    cmp al, 0               ; Update our flags to reflect the results of our VMREAD instruction

    ; Result in AL (0 = success, 1 = failed & status in vmcs, 2 = failed & no status)
ENDM

; rcx = base, rdx = limit
SwitchPrcbStackEntries PROC
    mov rax, gs:[188h]      ; rax = PKTHREAD CurrentThread
    mov [rax+38h], rcx      ; CurrentThread.StackBase
    mov [rax+30h], rdx      ; CurrentThread.StackLimit
    ret
SwitchPrcbStackEntries ENDP

GetPrcbStackEntries PROC
    mov rax, gs:[188h]
    mov rbx, [rax+38h]
    mov [rcx], rbx
    mov rbx, [rax+30h]
    mov [rdx], rbx
    ret
GetPrcbStackEntries ENDP

RawHandler PROC
    sub rsp, SIZEOF CONTEXT_FRAME_LENGTH    ; Allocate room on our stack for the context structure (doing this first for alignment for XMM operations)

    push rcx                                ; Preserve the original guest RCX value

    push rax                                ; Preserve RAX/RDI values
    push rdi

    xor rax, rax                            ; Zero our context structure
    lea rdi, [rsp+18h]                      ;
    mov rcx, SIZEOF CONTEXT_FRAME_LENGTH    ;
    rep stosb                               ;

    pop rdi                                 ; Restore RAX/RDI values
    pop rax

    lea rcx, [rsp+8]                        ; ContextRecord
    call RtlCaptureContext                  ; Save our guest state
    pop rcx                                 ; Restore our initial RCX value
    mov CxRcx[rsp], rcx                     ; Apply our preserved guest RCX value to the ContextRecord's RCX value

    lea rcx, [rsp]                          ; ContextRecord

    mov rdx, VMCS_GUEST_RSP                 ; Get/Set the guest SP
    _VMREAD                                 ;
    jne LErrorBreak                         ;
    mov CxRsp[rcx], r8                      ;

    mov rdx, VMCS_GUEST_RIP                 ; Get/Set the guest IP
    _VMREAD                                 ;
    jne LErrorBreak                         ;
    mov CxRip[rcx], r8                      ;

    push CxEFlags[rcx]                      ; Preserve host FLAGS

    mov rdx, VMCS_GUEST_RFLAGS              ; Get/Set the guest FLAGS
    _VMREAD                                 ;
    jne LErrorBreak                         ;
    mov CxEFlags[rcx], r8                   ;

    call VMExitHandler                      ; Call our C exit handler

    lea rcx, [rsp]                          ; ContextRecord
    pop CxEFlags[rcx]                       ; Restore host FLAGS

    mov rdx, LResume                        ; Obtain the location of our VMRESUME instruction
    lea rcx, [rsp]                          ; ContextRecord
    mov CxRip[rcx], rdx                     ; Set the ContextRecord's IP to match the location of our VMRESUME instruction

    mov rdx, VMCS_HOST_RSP                  ; Get our originally host SP, and use it for the ContextRecord's SP, removing the need to fixup the stack ourselves
    _VMREAD                                 ;
    jne LErrorBreak                         ;
    mov CxRsp[rcx], r8                      ;

    call RestoreContext                     ; Call our custom RtlRestoreContext, which is identical to the original, minus a call to , and support for an ExceptionRecord

LResume:
    VMRESUME                                ; Continue the guest execution (which implicitly loads the guest IP and SP)

LErrorBreak:
    INT 3                                   ; Break into the debugger if we reach this point

RawHandler ENDP

END
