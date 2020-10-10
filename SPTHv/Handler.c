#include "Handler.h"

PCONTEXT g_preLaunchCtx = NULL;

UINT64 g_GuestStackBase;
UINT64 g_GuestStackLimit;
UINT64 g_VMMStackBase;
UINT64 g_VMMStackLimit;
UINT64 g_InitialStackBase;
UINT64 g_InitialStackLimit;

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

    UINT8 guestStatus;
    UINT32 instrLen = 0;
    BOOLEAN fastReturn = FALSE;

	VM_EXIT_REASON exitReason;
	exitReason.All = 0;

    VM_ENTRY_INT_INFO_FIELD entryIntInfo;
    entryIntInfo.All = 0;

    // Set the CurrentThread's stack base and limit pair to those
    //  of the VMM (i.e. the current context)
    SwitchPrcbStackEntries( g_VMMStackBase, g_VMMStackLimit );

    PKTHREAD pCurrentThread;
    pCurrentThread = KeGetCurrentThread();

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

        case REASON_VMLAUNCH:
        {
            /*
             * Demonstrating event injection here
             *
             *  [26.5] "Event Injection"
             *  [24.8.3] "VM-Entry Controls for Event Injection"
             *  [3A, Chap. 6] "Interrupt and Exception Handling"
             *
             * How this works:
             * There is a 32-bit VMCS control called the VM-entry interrupt information
             *  field (0x4016) which allows you to deliver events through the IDT to
             *  underlying guest software. This technique is paramount to hypervisors,
             *  especially in the case of emulating instructions which may cause VM-exits.
             *  For instance: at any point in the execution of your guest software, if
             *  the VMLAUNCH instruction was executed, and we found ourselves in this
             *  block of code, you wouldn't want to just skip over the instruction.
             *  The guest would expect at least some sort of status indicator from this
             *  instruction, and that's where our event injection comes into play. In
             *  the spirit of wanting to retain a stable guest environment, event injection
             *  allows you to better emulate instructions such as VMLAUNCH to behave
             *  as they would outside of VMX operation (in cases where your hypervisor
             *  may not support nested virtualization), and inject either a #UD or #GP(0)
             *  (depending on the state of your guest software).
             *
             * Requirements for event injection:
             *  - The VMM must fill out an entry interrupt information structure
             *  (seen at [24.8.3] "VM-Entry Controls for Event Injeciton")
             *      - Most importantly, the valid bit must be set to deliver this event to the guest
             *  - Depending on the type of event you wish to inject, advice is given
             *      by intel on what interrupt type to use for your guest
             *  - It's also important to look up the instructions or events you're wishing
             *      to emulate in the SDM in order to ensure further information
             *      isn't required to be delivered to the guest (such as an exception error code)
             *  - The vector field below describes which handler (8-byte entry) in the IDT will
             *      receive control following a VM-entry
             *  (you can find these values in Vol.3A, [6.15] "Exception and Interrupt Reference")
             */

            __debugbreak();

            // If we're outside of VMX operation, VMLAUNCH generates a #UD before considering #GP(0)
            //  (see [Vol.3C, 30-14] "VMLAUNCH/VMRESUME—Launch/Resume Virtual Machine")
            entryIntInfo.Vector = INT_VEC_UD;

            // There's no exception error code generated for this exception
            //  (see [Vol.3A, 6-31] "Interrupt 6—Invalid Opcode Exception (#UD)"

            // Intel manual says to use type hardware exception for everything that's
            //  not a #BP or #OF (where it would use software exceptions)
            entryIntInfo.InterruptType = VM_INT_TYPE_HARDWARE_EXCEPTION;

            // Instruct the processor to deliver this event to the guest IDT
            entryIntInfo.Valid = TRUE;

            // Finally, write this entry interrupt information into our VMCS
            NT_ASSERT( __vmx_vmwrite(VMCS_CTRL_VM_ENTRY_INT_INFO_FIELD, entryIntInfo.All) == VMX_OK );

            // Can't just goto the quick return here, we need to increment
            //  RIP, so we set a local flag variable which allows us to do just that
            fastReturn = TRUE;

            break;
        }

		default:
        {
            // nope :(

            __debugbreak();

            LogMessage( L"Unhandled VM-exit reason (%d); guest IP = %p",
                exitReason.BasicReason, GuestContext->Rip );

            goto __exit_vmx_operations;
        }

	}

    // Advance the execution of the guest
    NT_ASSERT( __vmx_vmread(VMCS_RO_VM_EXIT_INSTR_LEN, (size_t*)&instrLen) == VMX_OK );

    GuestContext->Rip += instrLen;

    NT_ASSERT( __vmx_vmwrite(VMCS_GUEST_RIP, GuestContext->Rip) == VMX_OK );

    if (fastReturn == TRUE)
    {
        goto __quick_return_to_guest;
    }

    g_BreakCount += 1;

    __debugbreak();

    if ( g_BreakCount == 2 )
    {
        __debugbreak();

        LogMessage( L"Successfully finished guest execution (caught %d HLT instructions)", g_BreakCount );

        // Check the guest status to ensure our event injection succeeded
        guestStatus = *(UINT8*)((UINT64)GuestContext->Rsp + 0x28);

        if ( guestStatus == 0xBB )
        {
            LogMessage(L"Guest successfully received exception indicator!");
        }

        goto __exit_vmx_operations;
    }

    // Resume the guest
__quick_return_to_guest:

    // Set the CurrentThread stack base and limit pair to those of
    //  the guest stack
    SwitchPrcbStackEntries( g_GuestStackBase, g_GuestStackLimit );

    return;

__exit_vmx_operations:

    // Restore the CurrentThread base and limit pair to their
    //  original values
    SwitchPrcbStackEntries( g_InitialStackBase, g_InitialStackLimit );

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
