#include "Driver.h"

VOID
VMExitHandler()
{
    VM_EXIT_REASON exitReason;

    exitReason.All = 0;

    __vmx_vmread( VMCS_RO_EXIT_REASON, (size_t*)&exitReason.All );

    if ( exitReason.EntryFailure == TRUE )
    {
        __debugbreak();
        goto __jmp_driverentry_ep;
    }

    switch ( exitReason.BasicReason )
    {
        case REASON_HLT:

            // If we've broken here, this indicates a successful run-through
            //  the underlying guest software
            __debugbreak();

            break;
        default:
            __debugbreak();
            break;
    }

    __vmx_off();

__jmp_driverentry_ep:
    // Jump to our DriverEntry's function epilogue
    RtlRestoreContext( &g_preLaunchCtx, NULL );
}

VOID
_FixControlRegisters()
{
    // [23.7] "Enabling and Entering VMX Operation", [23.8] "Restrictions on VMX Operation"

    // Set the reserved bits of CR0
    __writecr0(
        FIX_BITS( g_CR0.All, __readmsr(IA32_VMX_CR0_FIXED1), __readmsr(IA32_VMX_CR0_FIXED0) )
        );

    // Set the reserved bits of CR4 (which implicitly enables VMX operations via setting the CR4.VMXE bit)
    __writecr4(
        FIX_BITS( g_CR4.All, __readmsr(IA32_VMX_CR4_FIXED1), __readmsr(IA32_VMX_CR4_FIXED0) )
        );
}

VOID
_SetVMCSGuestState(
    _In_ UINT64 GuestStack,
    _In_ UINT64 GuestEntryPoint
    )
{
    // [24.4] "Guest-State Area"

    // The guest stack must be 16-byte aligned
    NT_ASSERT( GuestStack % 16 == 0 );

    // [24.4.1] "Guest Register State"
    __vmx_vmwrite( VMCS_GUEST_CR0, __readcr0() );
    __vmx_vmwrite( VMCS_GUEST_CR3, __readcr3() );
    __vmx_vmwrite( VMCS_GUEST_CR4, __readcr4() );

    __vmx_vmwrite( VMCS_GUEST_DR7, __readdr(7) );

    __vmx_vmwrite( VMCS_GUEST_RSP, GuestStack );
    __vmx_vmwrite( VMCS_GUEST_RIP, GuestEntryPoint );

    __vmx_vmwrite( VMCS_GUEST_RFLAGS, __readeflags() );

    __vmx_vmwrite( VMCS_GUEST_CS_SELECTOR, __readcs() );
    __vmx_vmwrite( VMCS_GUEST_SS_SELECTOR, __readss() );
    __vmx_vmwrite( VMCS_GUEST_DS_SELECTOR, __readds() );
    __vmx_vmwrite( VMCS_GUEST_ES_SELECTOR, __reades() );
    __vmx_vmwrite( VMCS_GUEST_FS_SELECTOR, __readfs() );
    __vmx_vmwrite( VMCS_GUEST_GS_SELECTOR, __readgs() );
    __vmx_vmwrite( VMCS_GUEST_LDTR_SELECTOR, __readldtr() );
    __vmx_vmwrite( VMCS_GUEST_TR_SELECTOR, __readtr() );

    __vmx_vmwrite( VMCS_GUEST_CS_BASE, __segmentbase(__readcs()) );
    __vmx_vmwrite( VMCS_GUEST_SS_BASE, __segmentbase(__readss()) );
    __vmx_vmwrite( VMCS_GUEST_DS_BASE, __segmentbase(__readds()) );
    __vmx_vmwrite( VMCS_GUEST_ES_BASE, __segmentbase(__reades()) );
    __vmx_vmwrite( VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE) );
    __vmx_vmwrite( VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
    __vmx_vmwrite( VMCS_GUEST_LDTR_BASE, __segmentbase(__readldtr()) );
    __vmx_vmwrite( VMCS_GUEST_TR_BASE, __segmentbase(__readtr()) );

    __vmx_vmwrite( VMCS_GUEST_CS_LIMIT, __segmentlimit(__readcs()) );
    __vmx_vmwrite( VMCS_GUEST_SS_LIMIT, __segmentlimit(__readss()) );
    __vmx_vmwrite( VMCS_GUEST_DS_LIMIT, __segmentlimit(__readds()) );
    __vmx_vmwrite( VMCS_GUEST_ES_LIMIT, __segmentlimit(__reades()) );
    __vmx_vmwrite( VMCS_GUEST_FS_LIMIT, __segmentlimit(__readfs()) );
    __vmx_vmwrite( VMCS_GUEST_GS_LIMIT, __segmentlimit(__readgs()) );
    __vmx_vmwrite( VMCS_GUEST_LDTR_LIMIT, __segmentlimit(__readldtr()) );
    __vmx_vmwrite( VMCS_GUEST_TR_LIMIT, __segmentlimit(__readtr()) );

    __vmx_vmwrite( VMCS_GUEST_CS_ACCESS_RIGHTS, (ReadAR(__readcs())).All );
    __vmx_vmwrite( VMCS_GUEST_SS_ACCESS_RIGHTS, (ReadAR(__readss())).All );
    __vmx_vmwrite( VMCS_GUEST_DS_ACCESS_RIGHTS, (ReadAR(__readds())).All );
    __vmx_vmwrite( VMCS_GUEST_ES_ACCESS_RIGHTS, (ReadAR(__reades())).All );
    __vmx_vmwrite( VMCS_GUEST_FS_ACCESS_RIGHTS, (ReadAR(__readfs())).All );
    __vmx_vmwrite( VMCS_GUEST_GS_ACCESS_RIGHTS, (ReadAR(__readgs())).All );
    __vmx_vmwrite( VMCS_GUEST_LDTR_ACCESS_RIGHTS, (ReadAR(__readldtr())).All );
    __vmx_vmwrite( VMCS_GUEST_TR_ACCESS_RIGHTS, (ReadAR(__readtr())).All );

    __vmx_vmwrite( VMCS_GUEST_GDTR_BASE, g_GDTR.Base );
    __vmx_vmwrite( VMCS_GUEST_IDTR_BASE, g_IDTR.Base );

    __vmx_vmwrite( VMCS_GUEST_GDTR_LIMIT, g_GDTR.Limit );
    __vmx_vmwrite( VMCS_GUEST_IDTR_LIMIT, g_IDTR.Limit );

    __vmx_vmwrite( VMCS_GUEST_IA32_DEBUGCTL_FULL, __readmsr(IA32_DEBUGCTL) );
    __vmx_vmwrite( VMCS_GUEST_IA32_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS) );
    __vmx_vmwrite( VMCS_GUEST_IA32_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP) );
    __vmx_vmwrite( VMCS_GUEST_IA32_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP) );
}

VOID
_SetVMCSHostState(
    _In_ UINT64 HostStack,
    _In_ UINT64 HostEntryPoint
    )
{
    // [24.5] "Host-State Area"

    // The guest stack must be 16-byte aligned
    NT_ASSERT( HostStack % 16 == 0 );

    __vmx_vmwrite( VMCS_HOST_CR0, __readcr0() );
    __vmx_vmwrite( VMCS_HOST_CR3, __readcr3() );
    __vmx_vmwrite( VMCS_HOST_CR4, __readcr4() );

    __vmx_vmwrite( VMCS_HOST_RSP, HostStack );
    __vmx_vmwrite( VMCS_HOST_RIP, HostEntryPoint );

    /*
     * Note: "In the selector field for each of CS, SS, DS, ES, FS, GS and TR,
     *  the RPL (bits 1:0) and the TI flag (bit 2) must be 0"
     * (See [26.2.3] "Checks on Host Segment and Descriptor-Table Registers")
     */
    __vmx_vmwrite( VMCS_HOST_CS_SELECTOR, __readcs() & SELECTOR_INDEX_MASK );
    __vmx_vmwrite( VMCS_HOST_SS_SELECTOR, __readss() & SELECTOR_INDEX_MASK );
    __vmx_vmwrite( VMCS_HOST_DS_SELECTOR, __readds() & SELECTOR_INDEX_MASK );
    __vmx_vmwrite( VMCS_HOST_ES_SELECTOR, __reades() & SELECTOR_INDEX_MASK );
    __vmx_vmwrite( VMCS_HOST_FS_SELECTOR, __readfs() & SELECTOR_INDEX_MASK );
    __vmx_vmwrite( VMCS_HOST_GS_SELECTOR, __readgs() & SELECTOR_INDEX_MASK );
    __vmx_vmwrite( VMCS_HOST_TR_SELECTOR, __readtr() & SELECTOR_INDEX_MASK );

    __vmx_vmwrite( VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE) );
    __vmx_vmwrite( VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE) );

    __vmx_vmwrite( VMCS_HOST_TR_BASE, __segmentbase(__readtr()) );
    __vmx_vmwrite( VMCS_HOST_GDTR_BASE, g_GDTR.Base );
    __vmx_vmwrite( VMCS_HOST_IDTR_BASE, g_IDTR.Base );

    __vmx_vmwrite( VMCS_HOST_IA32_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS) );
    __vmx_vmwrite( VMCS_HOST_IA32_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP) );
    __vmx_vmwrite( VMCS_HOST_IA32_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP) );
}

VOID
_SetPinBasedControls()
{
    // [24.6.1] "Pin-Based VM-Execution Controls"

    PIN_VM_EXEC_CTRLS pinCtrls;
    pinCtrls.All = 0;

    // ...

    // Fix the control bits
    //  (Note: no pre-checking on allowed settings here)
    pinCtrls.All = FixCtrlBits( pinCtrls.All, IA32_VMX_PINBASED_CTRLS, IA32_VMX_TRUE_PINBASED_CTRLS );

    __vmx_vmwrite( VMCS_CTRL_PIN_EXEC_CTRLS, pinCtrls.All );
}

VOID
_SetProcessorPrimaryControls()
{
    // [24.6.2] "Processor-Based VM-Execution Controls"

    PROCESSOR_PRIMARY_VM_EXEC_CTRLS processorPrimaryCtrls;
    processorPrimaryCtrls.All = 0;

    // VM-exit on `HLT` instructions, which is the first instruction (by default) set in the GuestEntry function (see "./guest.asm")
    processorPrimaryCtrls.HLTExiting = 1; 

    // Use the provided MSR bitmap to determine when to cause VM-exits based on MSR read/write operations
    //    Note: by default this will ignore all MSR read/write operations, as no MSRs are specified in our bitmap (zeroed)
    processorPrimaryCtrls.UseMSRBitmaps = 1; 

    // Fix the control bits
    //  (Note: no pre-checking on allowed settings here)
    processorPrimaryCtrls.All = FixCtrlBits( processorPrimaryCtrls.All, IA32_VMX_PROCBASED_CTLS, IA32_VMX_TRUE_PROCBASED_CTLS );

    __vmx_vmwrite( VMCS_CTRL_PRIMARY_EXEC_CTRLS, processorPrimaryCtrls.All );
}

VOID
_SetProcessorSecondaryControls()
{
    // [24.6.2] "Processor-Based VM-Execution Controls"

    PROCESSOR_SECONDARY_VM_EXEC_CTRLS processorSecondaryCtrls;
    processorSecondaryCtrls.All = 0;

    // ...

    /*
     * Fix the control bits
     *  (Note: no pre-checking on allowed settings here.
     *  (Additionally, in the FixCtrlBits function I have included logic to handle the case of processor secondary controls)
     */
    processorSecondaryCtrls.All = FixCtrlBits( processorSecondaryCtrls.All, IA32_VMX_PROCBASED_CTLS2, 0 );

    __vmx_vmwrite( VMCS_CTRL_SECONDARY_EXEC_CTRLS, processorSecondaryCtrls.All );
}

VOID
_SetExitControls()
{
    // [24.7] "VM-Exit Control Fields"

    VM_EXIT_CTRLS exitCtrls;
    exitCtrls.All = 0;

    // We want to be in IA-32e mode (our current mode) on VM exits
    exitCtrls.HostAddressSpaceSize = 1;

    // Fix the control bits
    //  (Note: no pre-checking on allowed settings here)
    exitCtrls.All = FixCtrlBits( exitCtrls.All, IA32_VMX_EXIT_CTLS, IA32_VMX_TRUE_EXIT_CTLS );

    __vmx_vmwrite( VMCS_CTRL_VM_EXIT_CTRLS, exitCtrls.All );
}

VOID
_SetEntryControls()
{
    // [24.8] "VM-Entry Control Fields"

    VM_ENTRY_CTRLS entryCtrls;
    entryCtrls.All = 0;

    // Want the guest in IA-32e mode on VM entries
    entryCtrls.IA32eModeGuest = 1;

    // Fix the control bits
    //  (Note: no pre-checking on allowed settings here)
    entryCtrls.All = FixCtrlBits( entryCtrls.All, IA32_VMX_ENTRY_CTLS, IA32_VMX_TRUE_ENTRY_CTLS );

    __vmx_vmwrite( VMCS_CTRL_VM_ENTRY_CTRLS, entryCtrls.All );
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    KIRQL PreviousIRQL;

    BOOLEAN bSavedEpAddr = FALSE;
    UINT64 pEpAddr;

    VMX_BASIC_INFO vmxBasicInfo;
    FEATURE_CONTROL featureControl;

    UNREFERENCED_PARAMETER( DriverObject );
    UNREFERENCED_PARAMETER( RegistryPath );



    // Capture the GDT and IDT bases for later usage
    __sgdt( &g_GDTR );
    __sidt( &g_IDTR );

    // Capture the CR0/4 values for later usage
    g_CR0.All = __readcr0();
    g_CR4.All = __readcr4();



    // See [31.6] "Preparation and Launching a Virtual Machine" for "the minimal steps required by the VMM to set up and launch a guest VM"



    // 1. Allocate a stack for the VM, zero and initialize the allocations, and get it's physical address
    //    (Note: this isn't necessarily required for just the test, as the guest doesn't perform any stack operations)
    NT_ASSERT( utlAllocateVMXData( KERNEL_STACK_SIZE, FALSE, FALSE, &g_LPInfo.VMStack ) == TRUE );



    // 2. Allocate a stack for the VMM (host); this will get loaded on VM-exits, and be used by the exit handler
    NT_ASSERT( utlAllocateVMXData( KERNEL_STACK_SIZE, FALSE, FALSE, &g_LPInfo.HostStack ) == TRUE );



    // 3. Allocate an MSR bitmap (4KB contiguous physical address needed ([24.6.9] "MSR-Bitmap Address")
    //    (Note: this isn't *required*, but it's possible to generate a vm-exit with REASON_RDMSR at launch without it)
    NT_ASSERT( utlAllocateVMXData( PAGE_SIZE, TRUE, TRUE, &g_LPInfo.MSRBitmap ) == TRUE );



    // 4. Allocate the VMXON Region (4KB contiguous physical address needed, [24.11.5] "VMXON Region")
    NT_ASSERT( utlAllocateVMXData( VMX_ALLOCATION_DEFAULT_MAX, TRUE, TRUE, &g_LPInfo.VMXONRegion ) == TRUE );



    // 5. Allocate the VMCS Region (4KB contiguous physical address needed, [24.11.5] "VMXON Region")
    NT_ASSERT( utlAllocateVMXData( VMX_ALLOCATION_DEFAULT_MAX, TRUE, TRUE, &g_LPInfo.VMCS ) == TRUE );



    // 6. Assign revision identifiers to the above regions ([24.2] "Format of the VMCS Region", [24.11.5] "VMXON Region")
    vmxBasicInfo.All = __readmsr( IA32_VMX_BASIC );
    ((PVMXON_REGION)g_LPInfo.VMXONRegion.VA)->RevisionIdentifier = vmxBasicInfo.RevisionIdentifier;
    ((PVMCS)g_LPInfo.VMCS.VA)->RevisionIdentifier = vmxBasicInfo.RevisionIdentifier;



    // 7. Raise the IRQL to prevent context switches for this LP; as the following operations are specific to the current LP

    /*  Note: HIGH_LEVEL IRQL required here for how we're executing guest code.
     *
     *   Our VM-exit handler is effectively always at HIGH_LEVEL[0], but all of the other
     *   code, chiefly that which runs within VMX operation (like our guest), could potentially get
     *   context-switched by an interrupt running at a higher IRQL (such as device/clock interrupts);
     *   and this creates a particularly nasty bug with our implementation, as after the interrupt
     *   occurs, windows will try to use the RDTSCP function (in the call chain servicing the higher-IRQL
     *   interrupt), which isn't enabled in our processor secondary controls, which generates a #UD, and
     *   bugchecks the guest.
     *
     *  [0]: https://github.com/tandasat/HyperPlatform/issues/3#issuecomment-230494046
     */

    KeRaiseIrql( HIGH_LEVEL, &PreviousIRQL );



    // 8. Check feature MSRs for hypervisor system compatibility ([23.7] "Enabling and Entering VMX Operation")
    //    (Per the specification: the lock bit along with either VMXInsideSMX or VMXOutsideSMX must be set; and we only support VMX outside of SMX)
    featureControl.All = __readmsr( IA32_FEATURE_CONTROL );
    NT_ASSERT( featureControl.Lock == 1 && featureControl.VMXOutisdeSMX == 1 );



    // 9. Fix the CR0 and CR4 control registers to their required values for VMX operation
    _FixControlRegisters();



    // 10. Enter VMX operation
    NT_ASSERT( __vmx_on((UINT64*)&g_LPInfo.VMXONRegion.PA) == VMX_OK );



    // 11. Set launch state of VMCS to clear, render it inactive, and ensure all data is written (that may be cached by the processor)
    NT_ASSERT( __vmx_vmclear( (UINT64*)&g_LPInfo.VMCS.PA ) == VMX_OK );



    // 11. Load our VMCS onto the LP, and mark it active and current
    NT_ASSERT( __vmx_vmptrld( (UINT64*)&g_LPInfo.VMCS.PA ) == VMX_OK );



    // 12. Configure our VMCS sections ([24.3] "Organization of VMCS Data")

    // 12.1 Configure the guest state information ([24.4] "Guest-State Area")
    _SetVMCSGuestState(
        (UINT64)g_LPInfo.VMStack.VA + KERNEL_STACK_SIZE,
        (UINT64)GuestEntry
        );

    // 12.2 Configure the host state information ([24.5] "Host-State Area")
    _SetVMCSHostState(
        (UINT64)g_LPInfo.HostStack.VA + KERNEL_STACK_SIZE,
        (UINT64)VMExitHandler
        );

    // 12.3 Configure the VMCS control fields ([31.6] "Preparation and Launching a Virtual Machine")

    // 12.3.1 Configure the pin-based controls, and set fixed bits ([24.6.1], "Pin-Based VM-Execution Controls")
    //    (Note: we're not using this field for the time being, but in testing you may; so it's included here for posterity purposes)
    _SetPinBasedControls();
    
    // 12.1.2 Configure the primary processor controls ([24.6.2] "Processor-Based VM-Execution Controls")
    _SetProcessorPrimaryControls();

    // 12.1.3 Configure the processor secondary controls ([24.6.2] "Processor-Based VM-Execution Controls")
    //    (Note: we're not using this field for the time being, but in testing you may; so it's included here for posterity purposes)
    _SetProcessorSecondaryControls();
    
    // 12.4 Configure the VM-exit controls ([24.7] "VM-Exit Control Fields")
    _SetExitControls();

    // 12.5 Configure the VM-entry controls ([24.8] "VM-Entry Control Fields")
    _SetEntryControls();

    // 12.6 Set the VMCS link pointer to reflect our usage of the shadow VMCS ([26.3.1.5] "Checks on Guest Non-Register State")
    __vmx_vmwrite( VMCS_GUEST_VMCS_LINK_PTR_FULL, MAXUINT64 );

    // 12.7 Set the VMCS MSR bitmaps ([24.6.9] "MSR-Bitmap Address")
    __vmx_vmwrite( VMCS_CTRL_ADDR_MSR_BITMAPS_FULL, (UINT64)g_LPInfo.MSRBitmap.PA );



    // Hacky code here to capture the current state so that we can close out this function after our VM-exit
    //  and successfully load our driver without bugchecking the system
    RtlCaptureContext( &g_preLaunchCtx );
    goto __save_state;
__saved_ep_addr:
    g_preLaunchCtx.Rip = pEpAddr;



    // 13. Virtualize the LP (if this is successful, it will jump to VMExitHandler)
    __debugbreak();
    NT_ASSERT( __vmx_vmlaunch() == VMX_OK );



__save_state:
    // Save the address of the next instruction (the comparison operation below)
    utlGetNextInstrAddr( &pEpAddr );
    if ( bSavedEpAddr == FALSE )
    {
        bSavedEpAddr = TRUE;
        goto __saved_ep_addr;
    }

    __debugbreak();

    // Reset GDT/IDT limit to prevent PG bugchecks
    //   (Per the specification: there is no field for the current host GDT/IDT limits, so the hardware simply sets them to max, [] "")
    __lidt( &g_IDTR );
    __lgdt( &g_GDTR );

    // Restore CR0/4 states to pre-vmx operation
    __writecr0( g_CR0.All );
    __writecr4( g_CR4.All );



    // Cleanup



    // Restore PASSIVE_LEVEL IRQL
    KeLowerIrql( PreviousIRQL );

    KdPrint(( "[SPTHv] Successfully performed VMX operation cycle\r\n" ));

    // Free our allocated data structures
    utlFreeVMXData( &g_LPInfo.VMCS, TRUE );

    utlFreeVMXData( &g_LPInfo.VMXONRegion, TRUE );

    utlFreeVMXData( &g_LPInfo.MSRBitmap, TRUE );

    utlFreeVMXData( &g_LPInfo.HostStack, FALSE );

    utlFreeVMXData( &g_LPInfo.VMStack, FALSE );

    // Regardless of whether or not the driver was successful, there is no reason to keep it loaded.
    //    Seeing this status does not necessarily indicate an error.
    return STATUS_UNSUCCESSFUL;
}
