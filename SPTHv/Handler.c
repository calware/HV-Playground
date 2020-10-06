#include "Handler.h"

PCONTEXT g_preLaunchCtx = NULL;

EPT_SPLIT_REG stSplitRegistration = { 0 };

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

    UINT8 guestFunctionStatus;
    PHYSICAL_ADDRESS funcPa;
    UINT64 hookPhysPage, targetPhysPage;
    PEPT_PTE pTargetFnPte;

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

            if ( eptGuestPhysAddr == stSplitRegistration.TargetBaseAddress )
            {
                // Check if this was a read/write operation
                if ( eptViolationInfo.DataRead == TRUE || eptViolationInfo.DataWrite == TRUE )
                {
                    // If it's a R/W operation, flip the permission bit to allow R/W,
                    //  and map in our original page; which is what we wish to be reading from

                    stSplitRegistration.TargetPte->ReadAccess    =
                    stSplitRegistration.TargetPte->WriteAccess   = TRUE;

                    stSplitRegistration.TargetPte->BaseAddress   =
                        stSplitRegistration.TargetBaseAddress >> PAGE_OFFSET_4KB;
                }
                // Check if this was an instruction fetch operation
                else if ( eptViolationInfo.InstructionFetch == TRUE )
                {
                    // Flip the permission bits to allow execution,
                    //  and map the hook page; which what we wish to be executing

                    stSplitRegistration.TargetPte->ReadAccess    =
                    stSplitRegistration.TargetPte->WriteAccess   = FALSE;

                    stSplitRegistration.TargetPte->ExecuteAccess = TRUE;

                    stSplitRegistration.TargetPte->BaseAddress   =
                        stSplitRegistration.SwapBaseAddress >> PAGE_OFFSET_4KB;
                }
                else
                {
                    // We should never get to this point
                    NT_ASSERT( FALSE );
                }

                // We've made changes to the underlying PTEs, so we have to
                //  invalidate the TLB
                NT_ASSERT( EptInvalidateTlb(INVEPT_TYPE_GLOBAL_CONTEXT) == TRUE );
            }

            __debugbreak();

            goto __ept_fast_return_to_guest;
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
                /*
                 * EPT Splitting
                 *
                 * After we resume from the first HLT instruction, the guest will
                 *  read from the target function to ensure it contains the
                 *  expected bytes (0xC3AAB0 instead of 0xC3BBB0). It is our
                 *  main objective with EPT splitting to ensure that the guest
                 *  succeeds in it's integrity check operation (reading 0xC3AAB0),
                 *  while still executing the hook function (corresponding to
                 *  bytes 0xC3BBB0). This is done by a technique known as EPT
                 *  splitting.
                 *
                 * We can achieve this by simply supplying two different EPT pages
                 *  to the logical processor performing operations on the page
                 *  holding our target function. One of these EPT pages will have
                 *  the read/write bits set, allowing for IO operations on the
                 *  target page, while the other page will have the instruction
                 *  fetch bits set, allowing for execution on the target page.
                 *
                 * When the page allowing for read/write operations is present for
                 *  the EPT entry corresponding to the target page, instruction fetch
                 *  operations will prompt a VM-exit via an EPT violation (an attempt
                 *  to execute code on a page without instruction fetch permissions).
                 *  This is where we simply replace the page table entry with the one
                 *  that allows for instruction fetch operations, allowing guest
                 *  execution to continue. This process is exactly the same (albeit
                 *  reversed) for read/write operations on a page only allowing for
                 *  instruction fetch operations.
                 *
                 * One small deviation to this process is present in the code below.
                 *  You'll recall from our last branch that each of these functions
                 *  lies within their own respective pages, and this allows for
                 *  easy hooking of the target function via remapping the pages between
                 *  the target function and the hook function. Similarly, within our
                 *  EPT splitting demonstration, the page used for execution of our
                 *  guest target function will simply be that of the page containing
                 *  the guest hook funciton.
                 */

                __debugbreak();

                // Start by locating the target page

                funcPa = MmGetPhysicalAddress( (PVOID)GuestTargetFn );

                NT_ASSERT( funcPa.QuadPart != 0 );

                targetPhysPage = funcPa.QuadPart;
                 
                /*
                 *  Note: the  function makes use of several increasingly complicated
                 *  internal functions that will be used to split the 2MB large page PDE(s)
                 *  holding our target and hook functions into PT(s) of 4KB PTEs, and also
                 *  to invalidate the TLB per these changes.
                 */
                NT_ASSERT( EptGetPte(TRUE, funcPa, &pTargetFnPte) == TRUE );

                // Next, locate the page containing our hook function, and store it's
                //  address for future remapping

                funcPa = MmGetPhysicalAddress( (PVOID)GuestHookFn );

                NT_ASSERT( funcPa.QuadPart != 0 );

                hookPhysPage = funcPa.QuadPart;

                // Fill out a registration structure we created to keep track
                //  of this splitting instance

                //  First we want to preserve the original value (in case we wanted
                //   to stop splitting at some point
                stSplitRegistration.OriginalPte.All = pTargetFnPte->All;

                //  Next we want to save the physical page address for the target
                //   function for comparisons in our violation handler
                stSplitRegistration.TargetBaseAddress = targetPhysPage;

                //  Next we want to save the physical page address for the hook
                //   function for redirections in our violation handler
                stSplitRegistration.SwapBaseAddress = hookPhysPage;

                //  Lastly, we save the address of the EPT PTE identity mapping
                //   our target page for easy access later
                stSplitRegistration.TargetPte = pTargetFnPte;

                /*
                 * Now we change the permission bits for the target function to
                 *  disallow instruction fetches, leaving only read/write access
                 * (This is the first step in our splitting attack, and will prompt
                 *  future VM-exits on the basis of EPT violations)
                 */
                pTargetFnPte->ExecuteAccess = FALSE;

                /*
                 * We've now changed the permission bits of a PTE within our EPT
                 *  which may have been cached, and in that case wouldn't reflect
                 *  our changes, so we have to invalidate the TLB to reflect the
                 *  negation of execution access on the target page
                 */
                NT_ASSERT( EptInvalidateTlb(INVEPT_TYPE_GLOBAL_CONTEXT) == TRUE );

                /*
                 * At this point, the guest will continue, and succeed with it's future
                 *  read operation on the target page, but then attempt to execute the
                 *  target page where it will fail, prompting a VM-exit.
                 * (Seen above on line 80)
                 */

                LogMessage( L"Successfully initiated splitting operation on GuestTargetFn" );
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

    // Check if we've completed guest execution
    if ( g_BreakCount == 2 )
    {
        // Obtain the status value on the guest's stack
        guestFunctionStatus = *(UINT8*)((UINT64)GuestContext->Rsp + 0x28);

        if ( guestFunctionStatus == 0xBB )
        {
            LogMessage( L"Execution of GuestHookFn in place of GuestTargetFn succeeded" );
        }
        else if ( guestFunctionStatus == 0xAA )
        {
            LogMessage( L"GuestTargetFn not hooked!" );
        }
        else if ( guestFunctionStatus == 0xCC )
        {
            LogMessage( L"Guest detected modifications to GuestTargetFn and aborted execution!" );
        }

        LogMessage( L"Successfully finished guest execution (caught %d HLT instructions)", g_BreakCount );
        goto __exit_vmx_operations;
    }

__ept_fast_return_to_guest:
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
