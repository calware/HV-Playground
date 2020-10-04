#ifndef __MSR_H__
#define __MSR_H__

#include <wdm.h>


// [2.1] "Architectural MSRs", Table 2-2


// For obtaining basic VMX information; such as the revision identifier
#define IA32_VMX_BASIC                  0x480

// For checking system hypervisor compatability
#define IA32_FEATURE_CONTROL            0x3A

// For setting fixed control bits
#define IA32_VMX_CR0_FIXED0             0x486
#define IA32_VMX_CR0_FIXED1             0x487
#define IA32_VMX_CR4_FIXED0             0x488
#define IA32_VMX_CR4_FIXED1             0x489

// For the VMCS
#define IA32_DEBUGCTL                   0x1D9
#define IA32_SYSENTER_CS                0x174
#define IA32_SYSENTER_ESP               0x175
#define IA32_SYSENTER_EIP               0x176

// True control MSRs
#define IA32_VMX_PINBASED_CTRLS         0x481
#define IA32_VMX_TRUE_PINBASED_CTRLS    0x48D
#define IA32_VMX_PROCBASED_CTLS         0x482
#define IA32_VMX_TRUE_PROCBASED_CTLS    0x48E
#define IA32_VMX_PROCBASED_CTLS2        0x48B
#define IA32_VMX_EXIT_CTLS              0x483
#define IA32_VMX_TRUE_EXIT_CTLS         0x48F
#define IA32_VMX_ENTRY_CTLS             0x484
#define IA32_VMX_TRUE_ENTRY_CTLS        0x490

// Special segment MSRs
#define IA32_FS_BASE                    0xC0000100
#define IA32_GS_BASE                    0xC0000101
#define IA32_KERNEL_GS_BASE             0xC0000102

// EPT and VPID capability MSRs
#define IA32_VMX_EPT_VPID_CAP           0x48C

// [Table 2-2] "IA-32 Architectural MSRs (Contd.)"
#define IA32_MTRRCAP                    0xFE
#define IA32_MTRR_DEF_TYPE              0x2FF

// [11.11.2.2] "Fixed Range MTRRs"
//  Holds 8 64KB sub-ranges (512KB range, 00000-7FFFF)
#define IA32_MTRR_FIX64K_00000          0x250
//  Holds 16 16KB sub-ranges (256KB range, 80000-BFFFF)
#define IA32_MTRR_FIX16K_80000          0x258
#define IA32_MTRR_FIX16K_A0000          0x259
//  Holds 64 4KB sub-ranges (256KB range, C0000-FFFFF)
#define IA32_MTRR_FIX4K_C0000           0x268
#define IA32_MTRR_FIX4K_C8000           0x269
#define IA32_MTRR_FIX4K_D0000           0x26A
#define IA32_MTRR_FIX4K_D8000           0x26B
#define IA32_MTRR_FIX4K_E0000           0x26C
#define IA32_MTRR_FIX4K_E8000           0x26D
#define IA32_MTRR_FIX4K_F0000           0x26E
#define IA32_MTRR_FIX4K_F8000           0x26F

// [11.11.2.3] "Variable Range MTRRs"
//  Note: the remainder of the physbase/physmask MSR values
//  are increments of two past the two starting points below
#define IA32_MTRR_PHYSBASE0             0x200
#define IA32_MTRR_PHYSMASK0             0x201

#pragma warning(push)

#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int

// [A.1] "Basic VMX Information"
typedef union _VMX_BASIC_INFORMATION
{
    struct
    {
        UINT32 RevisionIdentifier : 31;                 // 0-30
        UINT64 Reserved0 : 1;                           // 31
        UINT64 VMAllocSize : 12;                        // 32-43
        UINT64 VMAllocMaxDefault : 1;                   // 44
        UINT64 Reserved1 : 3;                           // 45-47
        UINT64 VMXONPAWidth : 1;                        // 48
        UINT64 DualMonSMI : 1;                          // 49
        UINT64 MemType : 4;                             // 50-53    (A.1 "Basic VMX Information", Table A-1)
        UINT64 INSOUTSReporting : 1;                    // 54
        UINT64 TrueControls : 1;                        // 55        (Default1 controls can be zero, support for capability MSRs)
        UINT64 DeliverHardwareExcept : 1;               // 56
        // ...
    };
    UINT64 All;
} VMX_BASIC_INFO;

// [35.1] "IA-32 Architectural MSRs", Table 35-2
typedef union _FEATURE_CONTROL
{
    struct
    {
        UINT64 Lock : 1;                                // 0
        UINT64 VMXInsideSMX : 1;                        // 1
        UINT64 VMXOutisdeSMX : 1;                       // 2
        // ...
    };
    UINT64 All;
} FEATURE_CONTROL, *PFEATURE_CONTROL;

// [A.10] "VPID and EPT Capabilities"
#define CAP_INVEPT_SUPPORTED_MASK       0x6100000       // bits 20, 25, and 26
#define CAP_INVVPID_SUPPORTED_MASK      0xF0100000000   // bits 32, 40, 41, 42, and 43

#define IS_INVEPT_SUPPORTED(cap) ( ((cap & CAP_INVEPT_SUPPORTED_MASK) != 0) ? TRUE : FALSE )
#define IS_INVVPID_SUPPORTED(cap) ( ((cap & CAP_INVVPID_SUPPORTED_MASK) != 0) ? TRUE : FALSE )

typedef union _EPT_VPID_CAPABILITIES
{
    struct
    {
        UINT64 EPTExecuteOnly : 1;                      // 0
        UINT64 Reserved0 : 5;                           // 1-5
        UINT64 EPT4PageWalk : 1;                        // 6
        UINT64 Reserved1 : 1;                           // 7
        UINT64 EPTMemUC : 1;                            // 8
        UINT64 Reserved2 : 5;                           // 9-13
        UINT64 EPTMemWB : 1;                            // 14
        UINT64 Reserved3 : 1;                           // 15
        UINT64 EPTPage2MB : 1;                          // 16
        UINT64 EPTPage1GB : 1;                          // 17
        UINT64 Reserved4 : 1;                           // 18-19
        UINT64 INVEPT : 1;                              // 20
        UINT64 EPTAccessedDirty : 1;                    // 21
        UINT64 EPTViolationExtendedInformation : 1;     // 22
        UINT64 SupervisorShadowStack : 1;               // 23
        UINT64 Reserved5 : 1;                           // 24
        UINT64 INVEPTSingleContext : 1;                 // 25
        UINT64 INVEPTAllContext : 1;                    // 26
        UINT64 Reserved6 : 5;                           // 27-31
        UINT64 INVVPID : 1;                             // 32
        UINT64 Reserved7 : 7;                           // 33-39
        UINT64 INVVPIDIndividualAddress : 1;            // 40
        UINT64 INVVPIDSingleContext : 1;                // 41
        UINT64 INVVPIDAllContext : 1;                   // 42
        UINT64 INVVPIDSingleContextRetainGlobals : 1;   // 43
        // ...
    };
    UINT64 All;
} EPT_VPID_CAP;



#pragma warning(pop)

#endif // __MSR_H__
