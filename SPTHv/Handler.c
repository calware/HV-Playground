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

    UINT64 eptGuestLinAddr, eptGuestPhysAddr;
    EPT_VIOLATION_INFORMATION eptViolationInfo;

    PEPT_PTE pTargetEptPte = NULL;

    PHYSICAL_ADDRESS hookFnPA;
    hookFnPA.QuadPart = 0;


	__vmx_vmread( VMCS_RO_EXIT_REASON, (size_t*)&exitReason.All );

	if ( exitReason.EntryFailure == TRUE )
	{
        LogMessage( L"Unable to perform VM-entry; basicReason = %d", exitReason.BasicReason );
		goto __exit_vmx_operations;
	}

	switch ( exitReason.BasicReason )
	{
        case REASON_EPT_VIOLATION:
        {
            /*
             * I've left in this debugging information, so as to allow you to
             *  inspect and analyze EPT violations reported as VM exits.
             *
             * Generally, when there is a problem translating guest-physical
             *  addresses through our EPT, the page fault bit will be set (in
             *  this case that would be `eptViolation.EPTPageFault`), along with
             *  one of the access indicators (`DataRead`, `DataWrite`, or `InstructionFetch`).
             *
             * Additionally, it would be wise to inspect the guest linear address, and correlate it
             *  to the guest context information. This is how I solved several problems when developing
             *  the EPT paging entries. If the violation specifies there's a guest linear address (and
             *  the problem was a page fault), I would go through the `GuestContext` parameter, and using
             *  the linear address specified in `eptGuestLinAddr`, I would check if the linear address was
             *  the guest stack pointer (RSP), or guest instruction pointer (RIP), and then what kind of access
             *  it was, and then manually index the page tables using CR3 for that linear address, and compare
             *  the physical addresses indexed (each page table base address) to that of the `eptGuestPhysAddr`
             *  seen below. This allowed me to pinpoint the exact points at which the page fault
             *  occurred, and told me how to fix up the EPT to prevent this in the future.
             *
             * If you're having problems inserting entries into the EPT (where the processor has
             *  difficulty performing the translation—page faults) the translation processes
             *  is the same with standard paging tables, and there is a function within our
             *  EPT source files which reflects this idea (see `EptGetPteForSystemAddress`).
             *  When debugging my own EPT entries, I frequently used this function to check whether
             *  or not an address was indexable via simulating the translation mechanism.
             *
             * See also [Table 27-7] "Exit Qualification for EPT Violations", along with the information
             *  held below that table.
             */
            __vmx_vmread( VMCS_RO_EXIT_QUAL, (size_t*)&eptViolationInfo.All );
            __vmx_vmread( VMCS_RO_GUEST_PHYS_ADDR_FULL, (size_t*)&eptGuestPhysAddr );
            __vmx_vmread( VMCS_RO_GUEST_LIN_ADDR, (size_t*)&eptGuestLinAddr );

            __debugbreak();

            break;
        }
        case REASON_EPT_MISCONFIGURATION:
        {
            /*
             * Uncertain if this is recoverable by the guest or the host :(
             *
             * [28.2.3] "EPT-Induced VM Exits" reads, "an EPT misconfiguration occurs when,
             *  in the course of translating a gues-physical address, the logical processor
             *  encounters an EPT paging-structure entry that contains an unsupported value
             *  (see Section 28.2.3.1)".
             */
            __vmx_vmread( VMCS_RO_EXIT_QUAL, (size_t*)&eptViolationInfo.All );
            __vmx_vmread( VMCS_RO_GUEST_PHYS_ADDR_FULL, (size_t*)&eptGuestPhysAddr );
            __vmx_vmread( VMCS_RO_GUEST_LIN_ADDR, (size_t*)&eptGuestLinAddr );

            __debugbreak();

            goto __exit_vmx_operations;
        }

		case REASON_HLT:
        {
            if (g_BreakCount == 0)
            {
                // Obtain the EPT PTE for the desired target function
                NT_ASSERT( EptGetPteForSystemAddress((PVOID)GuestTargetFn, 0, &pTargetEptPte) == TRUE );

                // Obtain the physical address of our hook function
                hookFnPA = MmGetPhysicalAddress( (PVOID)GuestHookFn );

                NT_ASSERT( hookFnPA.QuadPart != 0 );

                /*
                 * Both `GuestTargetFn` and `GuestHookFn` begin at the start
                 *  of their respective pages; so we replace the page base address of `GuestTargetFn`
                 *  with `GuestHookFn` to effectively hook calls to `GuestTargetFn`
                 */
                pTargetEptPte->BaseAddress = hookFnPA.QuadPart >> PAGE_OFFSET_4KB;

                // At this point, we've successfully redirected the PTE for `GuestTargetFn` to point
                //  to the location of `GuestHookFn`, so we're done here

                /*
                 * Note: there is no need for an INVEPT instruction here, as the translation for the
                 *  guest-physical address which maps the `GuestTargetFn` hasn't succeeded up tot his point.
                 *
                 * If you were looking to perform an attack on the `GuestTargetFn` that hinged on the usage
                 *  of EPT violations (such as splitting, or permission-based breaks into the VMM), it may
                 *  be necessary to implement usage of the `INVEPT` function here in order to invalidate
                 *  the cached EPT translations residing in the TLB (assuming in this theoretical a prior
                 *  translation has succeeded).
                 */
            }

            LogMessage( L"Successfully caught VM-exit from our HLT instruction (%d); guest IP = %p",
                exitReason.BasicReason, GuestContext->Rip );
			break;
        }

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

    return;

__exit_vmx_operations:

    LogMessage( L"Cleaning up VMX operations..." );

    __debugbreak();

    // Exit VMX operations

	__vmx_off();

    // Jump to our DriverEntry's function epilogue
    //  using our specialized context restoration function

    // Defer to our custom RtlRestoreContext implementation
    //  (Note: this call leaves the VMM stack in an unclean state, so we shouldn't use [the VMM stack] anymore after this point)
    RestoreContext( g_preLaunchCtx );
}
