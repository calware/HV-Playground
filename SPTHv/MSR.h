#ifndef __MSR_H__
#define __MSR_H__

#include <wdm.h>


// [2.1] "Architectural MSRs", Table 2-2


// For obtaining basic VMX information; such as the revision identifier
#define IA32_VMX_BASIC					0x480

// For checking system hypervisor compatability
#define IA32_FEATURE_CONTROL			0x3A

// For setting fixed control bits
#define IA32_VMX_CR0_FIXED0				0x486
#define IA32_VMX_CR0_FIXED1				0x487
#define IA32_VMX_CR4_FIXED0				0x488
#define IA32_VMX_CR4_FIXED1				0x489

// For the VMCS
#define IA32_DEBUGCTL					0x1D9
#define IA32_SYSENTER_CS				0x174
#define IA32_SYSENTER_ESP				0x175
#define IA32_SYSENTER_EIP				0x176

// True control MSRs
#define IA32_VMX_PINBASED_CTRLS			0x481
#define IA32_VMX_TRUE_PINBASED_CTRLS	0x48D
#define IA32_VMX_PROCBASED_CTLS			0x482
#define IA32_VMX_TRUE_PROCBASED_CTLS	0x48E
#define IA32_VMX_PROCBASED_CTLS2		0x48B
#define IA32_VMX_EXIT_CTLS				0x483
#define IA32_VMX_TRUE_EXIT_CTLS			0x48F
#define IA32_VMX_ENTRY_CTLS				0x484
#define IA32_VMX_TRUE_ENTRY_CTLS		0x490

// Fuck my whole ass
#define IA32_FS_BASE					0xC0000100
#define IA32_GS_BASE 					0xC0000101
#define IA32_KERNEL_GS_BASE 			0xC0000102


#pragma warning(push)

#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int

// [A.1] "Basic VMX Information"
typedef union _VMX_BASIC_INFORMATION {
	struct {
		UINT32 /*[sic]*/ RevisionIdentifier : 31;	// 0-30
		UINT64 Reserved0 : 1;						// 31
		UINT64 VMAllocSize : 12;					// 32-43
		UINT64 VMAllocMaxDefault : 1;				// 44
		UINT64 Reserved1 : 3;						// 45-47
		UINT64 VMXONPAWidth : 1;					// 48
		UINT64 DualMonSMI : 1;						// 49
		UINT64 VMCSMemType : 4;						// 50-53	(A.1 "Basic VMX Information", Table A-1)
		UINT64 INSOUTSReporting : 1;				// 54
		UINT64 TrueControls : 1;					// 55		(Default1 controls can be zero, support for capability MSRs)
		UINT64 DeliverHardwareExcept : 1;			// 56
		// ...
	};
	UINT64 All;
} VMX_BASIC_INFO;

// [35.1] "IA-32 Architectural MSRs", Table 35-2
typedef union _FEATURE_CONTROL
{
	struct {
		UINT64 Lock : 1;							// 0
		UINT64 VMXInsideSMX : 1;					// 1
		UINT64 VMXOutisdeSMX : 1;					// 2
		// ...
	};
	UINT64 All;
} FEATURE_CONTROL, *PFEATURE_CONTROL;

#pragma warning(pop)

#endif // __MSR_H__
