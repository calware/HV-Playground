#include "Handler.h"

PCONTEXT g_preLaunchCtx = NULL;

VOID
VMExitHandler(
    _In_ PCONTEXT CONST GuestContext
    )
{
    /*
     * Note: Modification of GuestContext members that have dedicated VMCS
     *  fields will result in unchanged values on vm-entry. To change these
     *  values, you will need to perform VMWRITE operations on the VMCS.
     *  Notable values in this category include the guest IP, SP, and FLAGS registers.
     */

    UINT32 instrLen = 0;

	VM_EXIT_REASON exitReason;
	exitReason.All = 0;

    UNREFERENCED_PARAMETER( GuestContext );

	__vmx_vmread( VMCS_RO_EXIT_REASON, (size_t*)&exitReason.All );

	if ( exitReason.EntryFailure == TRUE )
	{
        LogMessage( L"Unable to perform VM-entry; basicReason = %d", exitReason.BasicReason );
		goto __exit_vmx_operations;
	}

	switch ( exitReason.BasicReason )
	{
		case REASON_HLT:
            LogMessage( L"Successfully caught VM-exit from our HLT instruction (%d); guest IP = %p",
                exitReason.BasicReason, GuestContext->Rip );
			break;
		default:
            LogMessage( L"Unhandled VM-exit reason (%d); guest IP = %p",
                exitReason.BasicReason, GuestContext->Rip );
            goto __exit_vmx_operations;
	}

    // Advance the execution of the guest
    NT_ASSERT( __vmx_vmread(VMCS_RO_VM_EXIT_INSTR_LEN, (size_t*)&instrLen) == VMX_OK );

    GuestContext->Rip += instrLen;

    NT_ASSERT( __vmx_vmwrite(VMCS_GUEST_RIP, GuestContext->Rip) == VMX_OK );

    g_BreakCount += 1;

    __debugbreak();

    if ( g_BreakCount == 2 )
    {
        LogMessage( L"Successfully finished guest execution (caught %d HLT instructions)", g_BreakCount );
        goto __exit_vmx_operations;
    }

    // Resume the guest
    return;

__exit_vmx_operations:

    LogMessage( L"Cleaning up VMX operations..." );

    __debugbreak();

    // Exit VMX operations

	__vmx_off();

    // Jump to our DriverEntry's function epilogue

    // This will try to execute ZwContinue instead of running through the main
    //  function body (which is ineffective because of our HIGH_LEVEL IRQL)
	// RtlRestoreContext( &g_preLaunchCtx, NULL );

    // Defer to our custom RtlRestoreContext implementation
    //  (Note: this call leaves the VMM stack in an unclean state, so we shouldn't use [the VMM stack] anymore after this point)
    RestoreContext( g_preLaunchCtx );
}
