#include "VMX.h"

UINT32
FixCtrlBits(
	_In_ UINT32 CtrlVal,
	_In_ UINT32 StandardMSR,
	_In_ UINT32 TrueMSR
	)
{
	VMX_BASIC_INFO basicInfo = { 0 };

	basicInfo.All = _MSR( IA32_VMX_BASIC );

	if ( basicInfo.TrueControls == 0 || StandardMSR == IA32_VMX_PROCBASED_CTLS2 )
	{
		return _FIX_CTRL_BITS(CtrlVal, _MSR(StandardMSR));
	}

	return _FIX_CTRL_BITS(CtrlVal, _MSR(TrueMSR));
}
