#ifndef __VMX_H__
#define __VMX_H__

#include <intrin.h>
#include "MSR.h"

#define VMX_ALLOCATION_DEFAULT_MAX 0x1000

#define FIX_BITS(v, o, z) ( (v & o) | z )
/*
 * The `FIX_BITS` macro allows us to toggle allowed and disallowed bits via one constant
 *	principle used throughout control fields within the SDM where such fixed values are required:
 *	"If bit X is 1 in IA32_VMX_CR0_FIXED0, then that bit of CR0 is fixed to 1 ... if bit X is 0
 *	  in IA32_VMX_CR0_FIXED1, then that bit of CR0 is fixed to 0" - [A.7] "VMX-Fixed Bits in CR0 (pg.4514)
 *
 *	An algorithm commonly seen to pass this check on fixed bits is as follows:
 *		Value &= Fixed1;
 *		Value |= Fixed0;
 *	This works, as the first operation will unset all bits which aren't allowed to be set, while
 *		the second operation will set all bits that are required to be set. Different variations of this
 *		operation are possible.
 *	You can see the above example in the Linux kernel's VMX header here:
 *	https://github.com/torvalds/linux/blob/c6dd78fcb8eefa15dd861889e0f59d301cb5230c/tools/testing/selftests/kvm/lib/x86_64/vmx.c#L89-L104
 *
 *	Ignoring the specific MSRs mentioned in the excerpt above, this principle is constant across the VMCS fixed
 *		bits as well; for example, see IA32_VMX_PINBASED_CTLS versus IA32_VMX_TRUE_PINBASED_CTRLS
 *		([A.3.1] "Pin-Based VM-Execution Controls"—broader information within parent at [A.3] "VM-Execution Controls")
 *
 *	See also:
 *		[A.8] "VMX-Fixed Bits in CR4" (pg.4514)
 *		[A.2] "Reserved Controls and Default Settings" (pg.4510)
 */

// #define _FIX_CTRL_BITS(v, f) ( (v | (UINT32)f) & (UINT32)(f >> 32) )
#define _FIX_CTRL_BITS(v, f) ( (v & (UINT32)(f >> 32)) | (UINT32)f ) // CHANGE HERE!
/*
 * The `_FIX_CTRL_BITS` macro does the same thing as `FIX_BITS`, but using a 64-bit fixed
 *   value, which is really two 32-bit fixed values (0s and 1s, respectively), returned via an MSR.
 *   This is to account for the standard fixed bits for VMCS controls versus the 'true' fixed bits (which allow
 *   future features to be toggled, which may require bits to unset in order to toggle.
 *
 * Note: this macro is only used internally via the FixCtrlBits function below
 */

#define _MSR(msr) __readmsr(msr)

#define VMX_SUCCESS(status) ( status == VMX_OK )

// See microsoft intrinsic functions (Ex: https://docs.microsoft.com/en-us/cpp/intrinsics/vmx-on?view=vs-2019)
typedef enum _VMX_STATUS_CODE {
	VMX_OK,
	VMX_ERROR_STATUS,
	VMX_ERROR
} VMX_STATUS_CODE;

// [30.4] "VM Instruction Error Numbers"
//	(Read via VMREAD of VMCS_RO_VM_INSTR_ERR)
typedef enum _VM_INSTR_ERROR {
	VM_INSTR_ERROR_VMCALL_IN_VMX_ROOT = 1,
	VM_INSTR_ERROR_VMCLEAR_WITH_INVALID_PHYS_ADDR,
	VM_INSTR_ERROR_VMCLEAR_WITH_VMXON_PTR,
	VM_INSTR_ERROR_VMLAUNCH_WITH_NON_CLEAR_VMCS,
	VM_INSTR_ERROR_VMRESUME_WITH_NON_LAUNCHED_VMCS,
	VM_INSTR_ERROR_VMRESUME_AFTER_VMXOFF,
	VM_INSTR_ERROR_VM_ENTRY_WITH_INVALID_CTRL_FIELDS,
	VM_INSTR_ERROR_VM_ENTRY_WITH_INVALID_HOST_STATE_FIELDS,
	VM_INSTR_ERROR_VMPTRLD_WITH_INVALID_PHYS_ADDR,
	VM_INSTR_ERROR_VMPTRLD_WITH_VMXON_PTR,
	VM_INSTR_ERROR_VMPTRLD_WITH_INCORRECT_VMCS_REV_ID,
	VM_INSTR_ERROR_VMREADWRITE_FROMTO_UNSUPPORTED_VMCS_COMPONENT,
	VM_INSTR_ERROR_VMWRITE_TO_READONLY_VMCS_COMPONENT,
	VM_INSTR_ERROR_VMXON_IN_VMX_ROOT = 15,
	VM_INSTR_ERROR_VM_ENTRY_WITH_INVALID_VMCS_PTR,
	VM_INSTR_ERROR_VM_ENTRY_WITH_NON_LAUNCHED_VMCS,
	VM_INSTR_ERROR_VM_ENTRY_WITH_VMCS_PTR_NOT_VMXON,
	VM_INSTR_ERROR_VMCALL_WITH_NON_CLEAR_VMCS,
	VM_INSTR_ERROR_VMCALL_WITH_INVALID_VM_EXIT_CTRL_FIELDS,
	VM_INSTR_ERROR_VMCALL_WITH_INCORRECT_MSEG_REV_ID = 22,
	VM_INSTR_ERROR_VMXOFF_UNDER_DUAL_MON,
	VM_INSTR_ERROR_VMCALL_WITH_INVALID_SMM_FEATURES,
	VM_INSTR_ERROR_VM_ENTRY_WITH_INVALID_VM_EXEC_CTRL_FIELDS,
	VM_INSTR_ERROR_VM_ENTRY_EVENTS_BLOCKED_BY_MOV_SS,
	VM_INSTR_ERROR_INVALID_OPERAND_TO_INVEPT_INVVPID
} VM_INSTR_ERROR;

// [Appendix C] "VMX Basic Exit Reasons", Table C-1
typedef enum _VMX_BASIC_EXIT_REASON {
	REASON_EXCEPTION_OR_NMI,
	REASON_EXTERNAL_INTERRUPT,
	REASON_TRIPLE_FAULT,
	REASON_INIT_SIGNAL,
	REASON_STARTUP_IPI,
	REASON_IO_SMI,
	REASON_OTHER_SMI,
	REASON_INTERRUPT_WINDOW,
	REASON_NMI_WINDOW,
	REASON_TASK_SWITCH,
	REASON_CPUID,
	REASON_GETSEC,
	REASON_HLT,
	REASON_INVD,
	REASON_INVLPG,
	REASON_RDPMC,
	REASON_RDTSC,
	REASON_RSM,
	REASON_VMCALL,
	REASON_VMCLEAR,
	REASON_VMLAUNCH,
	REASON_VMPTRLD,
	REASON_VMPTRST,
	REASON_VMREAD,
	REASON_VMRESUME,
	REASON_VMWRITE,
	REASON_VMXOFF,
	REASON_VMXON,
	REASON_CONTROL_REGISTER_ACCESS,
	REASON_DEBUG_REGISTER_ACCESS,
	REASON_IO_INSTRUCTION,
	REASON_RDMSR,
	REASON_WRMSR,
	REASON_GUEST_STATE_ENTRY_FAILURE,
	REASON_MSR_LOADING_ENTRY_FAILURE,
	REASON_MWAIT = 36,
	REASON_MONITOR_TRAP_FLAG,
	REASON_MONITOR = 39,
	REASON_PAUSE,
	REASON_MACHINE_CHECK_ENVENT_ENTRY_FAILURE,
	REASON_TPR_BELOW_THRESHOLD = 43,
	REASON_APIC_ACCESS,
	REASON_VIRTUALIZED_EOI,
	REASON_GDTR_IDTR_ACCESS,
	REASON_LDTR_TR_ACCESS,
	REASON_EPT_VIOLATION,
	REASON_EPT_MISCONFIGURATION,
	REASON_INVEPT,
	REASON_RDTSCP,
	REASON_PREEMPTION_TIMER_EXPIRE,
	REASON_INVVPID,
	REASON_WBINVD,
	REASON_XSETBV,
	REASON_APIC_WRITE,
	REASON_RDRAND,
	REASON_INVPCID,
	REASON_VMFUNC,
	REASON_ENCLS,
	REASON_RDSEED,
	REASON_PML_LOG_FULL,
	REASON_XSAVES,
	REASON_XSTORS,
	REASON_SPP_EVENT = 66,
	REASON_UMWAIT,
	REASON_TPAUSE
} VMX_BASIC_EXIT_REASON;

UINT32
FixCtrlBits(
	_In_ UINT32 CtrlVal,
	_In_ UINT32 StandardMSR,
	_In_ UINT32 TrueMSR
	);

#endif // __VMX_H__
