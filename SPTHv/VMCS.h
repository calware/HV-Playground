#ifndef __VMCS_H__
#define __VMCS_H__

#include <wdm.h>

/**
  *  The following definitions are `VMCS encodings`, which you can read more about in the section titled "VMREAD, VMWRITE, and Encodings of VMCS Fields" within volume 3C of the SDM, section 24.11.2
  *  
  *  In short, they are just values fed into the `VMWRITE` and `VMREAD` commands to set fields in the *current* VMCS.
  *    Current, in this sense, meaning one which was already loaded via VMPTRLD.
  */

/* In addition to the above reference to section 24.11.2 in volume 3C, you can find tables in which the below definitions were derived from in the section titled "Field Encoding in VMCS" within volume 3D (a specialized document containing appendices for volume 3C), appendix B; with the various subsections of the appendix linked below. */


/* 16-bit Control Fields (B.1.1) */
#define VMCS_CTRL_VPID                              0x0000    // Must have "enable VPID" bit set
#define VMCS_CTRL_POSTED_INT_VEC                    0x0002    // Must have "process posted interrupts" bit set
#define VMCS_CTRL_EPTP_INDEX                        0x0004    // Must have "EPT-violation" bit set


/* 16-Bit Guest State Fields (B.1.2) */
#define VMCS_GUEST_ES_SELECTOR                      0x0800
#define VMCS_GUEST_CS_SELECTOR                      0x0802
#define VMCS_GUEST_SS_SELECTOR                      0x0804
#define VMCS_GUEST_DS_SELECTOR                      0x0806
#define VMCS_GUEST_FS_SELECTOR                      0x0808
#define VMCS_GUEST_GS_SELECTOR                      0x080A
#define VMCS_GUEST_LDTR_SELECTOR                    0x080C
#define VMCS_GUEST_TR_SELECTOR                      0x080E
#define VMCS_GUEST_INT_STATUS                       0x0810    // Must have "virtual-interrupt delivery" bit set
#define VMCS_GUEST_PML_INDEX                        0x0812    // Must have "enable PML" bit set


/* 16-Bit Host State Fields (B.1.3) */
#define VMCS_HOST_ES_SELECTOR                       0x0C00
#define VMCS_HOST_CS_SELECTOR                       0x0C02
#define VMCS_HOST_SS_SELECTOR                       0x0C04
#define VMCS_HOST_DS_SELECTOR                       0x0C06
#define VMCS_HOST_FS_SELECTOR                       0x0C08
#define VMCS_HOST_GS_SELECTOR                       0x0C0A
#define VMCS_HOST_TR_SELECTOR                       0x0C0C


/* 64-Bit Control Fields (B.2.1) */
#define VMCS_CTRL_ADDR_IO_BITMAP_A_FULL             0x2000
#define VMCS_CTRL_ADDR_IO_BITMAP_A_HIGH             0x2001
#define VMCS_CTRL_ADDR_IO_BITMAP_B_FULL             0x2002
#define VMCS_CTRL_ADDR_IO_BITMAP_B_HIGH             0x2003
#define VMCS_CTRL_ADDR_MSR_BITMAPS_FULL             0x2004    // Must have "use MSR bitmaps" bit set
#define VMCS_CTRL_ADDR_MSR_BITMAPS_HIGH             0x2005    // Must have "use MSR bitmaps" bit set
#define VMCS_CTRL_VM_EXIT_MSR_STORE_ADDR_FULL       0x2006
#define VMCS_CTRL_VM_EXIT_MSR_STORE_ADDR_HIGH       0x2007
#define VMCS_CTRL_VM_EXIT_MSR_LOAD_ADDR_FULL        0x2008
#define VMCS_CTRL_VM_EXIT_MSR_LOAD_ADDR_HIGH        0x2009
#define VMCS_CTRL_VM_ENTRY_MSR_LOAD_ADDR_FULL       0x200A
#define VMCS_CTRL_VM_ENTRY_MSR_LOAD_ADDR_HIGH       0x200B
#define VMCS_CTRL_EXECUTIVE_VMCS_POINTER_FULL       0x200C
#define VMCS_CTRL_EXECUTIVE_VMCS_POINTER_HIGH       0x200D
#define VMCS_CTRL_PML_ADDR_FULL                     0x200E    // Must have "enable PML" bit set
#define VMCS_CTRL_PML_ADDR_HIGH                     0x200F    // Must have "enable PML" bit set
#define VMCS_CTRL_TSC_OFFSET_FULL                   0x2010
#define VMCS_CTRL_TSC_OFFSET_HIGH                   0x2011
#define VMCS_CTRL_VIRT_APIC_ADDR_FULL               0x2012    // Must have "use TPR shadow" bit set
#define VMCS_CTRL_VIRT_APIC_ADDR_HIGH               0x2013    // Must have "use TPR shadow" bit set
#define VMCS_CTRL_APIC_ACCESS_ADDR_FULL             0x2014    // Must have "virtualize APIC accesses" bit set
#define VMCS_CTRL_APIC_ACCESS_ADDR_HIGH             0x2015    // Must have "virtualize APIC accesses" bit set
#define VMCS_CTRL_POSTED_INT_DESC_ADDR_FULL         0x2016    // Must have "process posted interrupts" bit set
#define VMCS_CTRL_POSTED_INT_DESC_ADDR_HIGH         0x2017    // Must have "process posted interrupts" bit set
#define VMCS_CTRL_VM_FUNC_CTRLS_FULL                0x2018    // Must have "enable VM functions" bit set
#define VMCS_CTRL_VM_FUNC_CTRLS_HIGH                0x2019    // Must have "enable VM functions" bit set
#define VMCS_CTRL_EPT_POINTER_FULL                  0x201A    // Must have "enable EPT" bit set
#define VMCS_CTRL_EPT_POINTER_HIGH                  0x201B    // Must have "enable EPT" bit set
#define VMCS_CTRL_EOI_EXIT_BITMAP_0_FULL            0x201C    // Must have "virtual-interrupt delivery" bit set
#define VMCS_CTRL_EOI_EXIT_BITMAP_0_HIGH            0x201D    // Must have "virtual-interrupt delivery" bit set
#define VMCS_CTRL_EOI_EXIT_BITMAP_1_FULL            0x201E    // Must have "virtual-interrupt delivery" bit set
#define VMCS_CTRL_EOI_EXIT_BITMAP_1_HIGH            0x201F    // Must have "virtual-interrupt delivery" bit set
#define VMCS_CTRL_EOI_EXIT_BITMAP_2_FULL            0x2020    // Must have "virtual-interrupt delivery" bit set
#define VMCS_CTRL_EOI_EXIT_BITMAP_2_HIGH            0x2021    // Must have "virtual-interrupt delivery" bit set
#define VMCS_CTRL_EOI_EXIT_BITMAP_3_FULL            0x2022    // Must have "virtual-interrupt delivery" bit set
#define VMCS_CTRL_EOI_EXIT_BITMAP_3_HIGH            0x2023    // Must have "virtual-interrupt delivery" bit set
#define VMCS_CTRL_EPTP_LIST_ADDR_FULL               0x2024    // Must have "EPTP switching" bit set
#define VMCS_CTRL_EPTP_LIST_ADDR_HIGH               0x2025    // Must have "EPTP switching" bit set
#define VMCS_CTRL_VMREAD_BITMAP_ADDR_FULL           0x2026    // Must have "VMCS shadowing" bit set
#define VMCS_CTRL_VMREAD_BITMAP_ADDR_HIGH           0x2027    // Must have "VMCS shadowing" bit set
#define VMCS_CTRL_VMWRITE_BITMAP_ADDR_FULL          0x2028    // Must have "VMCS shadowing" bit set
#define VMCS_CTRL_VMWRITE_BITMAP_ADDR_HIGH          0x2029    // Must have "VMCS shadowing" bit set
#define VMCS_CTRL_VIRT_EXCEPT_INFO_ADDR_FULL        0x202A    // Must have "EPT-violation #VE" bit set
#define VMCS_CTRL_VIRT_EXCEPT_INFO_ADDR_HIGH        0x202B    // Must have "EPT-violation #VE" bit set
#define VMCS_CTRL_XSS_EXITING_BITMAP_FULL           0x202C    // Must have "enable XSAVES/XRSTORS" bit set
#define VMCS_CTRL_XSS_EXITING_BITMAP_HIGH           0x202D    // Must have "enable XSAVES/XRSTORS" bit set
#define VMCS_CTRL_ENCLS_EXITING_BITMAP_FULL         0x202E    // Must have "enable ENCLS exiting" bit set
#define VMCS_CTRL_ENCLS_EXITING_BITMAP_HIGH         0x202F    // Must have "enable ENCLS exiting" bit set
#define VMCS_CTRL_SUBPAGE_PERM_TABLE_PTR_FULL       0x2030    // Must have "sub-page write permissions for EPT" bit set
#define VMCS_CTRL_SUBPAGE_PERM_TABLE_PTR_HIGH       0x2031    // Must have "sub-page write permissions for EPT" bit set
#define VMCS_CTRL_TSC_MULTIPLIER_FULL               0x2032    // Must have "use TSC scaling" bit set
#define VMCS_CTRL_TSC_MULTIPLIER_HIGH               0x2033    // Must have "use TSC scaling" bit set
#define VMCS_CTRL_ENCLV_EXITING_BITMAP_FULL         0x2036    // Must have "enable ENCLV exiting" bit set
#define VMCS_CTRL_ENCLV_EXITING_BITMAP_HIGH         0x2037    // Must have "enable ENCLV exiting" bit set


/* 64-Bit Read-Only Data Fields (B.2.2) */
#define VMCS_RO_GUEST_PHYS_ADDR_FULL                0x2400    // Must have "enable EPT" bit set
#define VMCS_RO_GUEST_PHYS_ADDR_HIGH                0x2401    // Must have "enable EPT" bit set


/* 64-Bit Guest-State Fields (B.2.3) */
#define VMCS_GUEST_VMCS_LINK_PTR_FULL               0x2800
#define VMCS_GUEST_VMCS_LINK_PTR_HIGH               0x2801
#define VMCS_GUEST_IA32_DEBUGCTL_FULL               0x2802
#define VMCS_GUEST_IA32_DEBUGCTL_HIGH               0x2803    // May be unused!
#define VMCS_GUEST_IA32_PAT_FULL                    0x2804    // Must have "load IA32_PAT" bit set
#define VMCS_GUEST_IA32_PAT_HIGH                    0x2805    // Must have "load IA32_PAT" bit set
#define VMCS_GUEST_IA32_EFER_FULL                   0x2806    // Must have "load IA32_EFER" bit set
#define VMCS_GUEST_IA32_EFER_HIGH                   0x2807    // Must have "load IA32_EFER" bit set
#define VMCS_GUEST_IA32_PERF_GLB_CTRL_FULL          0x2808    // Must have "load IA32_PERF_GLOBAL_CTRL" bit set
#define VMCS_GUEST_IA32_PERF_GLB_CTRL_HIGH          0x2809    // Must have "load IA32_PERF_GLOBAL_CTRL" bit set
#define VMCS_GUEST_PDPTE_0_FULL                     0x280A    // Must have "enable EPT" bit set
#define VMCS_GUEST_PDPTE_0_HIGH                     0x280B    // Must have "enable EPT" bit set
#define VMCS_GUEST_PDPTE_1_FULL                     0x280C    // Must have "enable EPT" bit set
#define VMCS_GUEST_PDPTE_1_HIGH                     0x280D    // Must have "enable EPT" bit set
#define VMCS_GUEST_PDPTE_2_FULL                     0x280E    // Must have "enable EPT" bit set
#define VMCS_GUEST_PDPTE_2_HIGH                     0x280F    // Must have "enable EPT" bit set
#define VMCS_GUEST_PDPTE_3_FULL                     0x2810    // Must have "enable EPT" bit set
#define VMCS_GUEST_PDPTE_3_HIGH                     0x2811    // Must have "enable EPT" bit set
#define VMCS_GUEST_IA32_BNDCFGS_FULL                0x2812    // Must have "load IA32_BNDCFGS" bit set
#define VMCS_GUEST_IA32_BNDCFGS_HIGH                0x2813    // Must have "load IA32_BNDCFGS" bit set
#define VMCS_GUEST_IA32_RTIT_CTL_FULL               0x2814    // Must have "load/clear IA32_RTIT_CTL" bit set
#define VMCS_GUEST_IA32_RTIT_CTL_HIGH               0x2815    // Must have "load/clear IA32_RTIT_CTL" bit set
#define VMCS_GUEST_IA32_PKRS_FULL                   0x2818    // Must have "load PKRS" bit set
#define VMCS_GUEST_IA32_PKRS_HIGH                   0x2819    // Must have "load PKRS" bit set


/* 64-Bit Host-State Fields (B.2.4) */
#define VMCS_HOST_IA32_PAT_FULL                     0x2C00    // Must have "load IA32_PAT" bit set
#define VMCS_HOST_IA32_PAT_HIGH                     0x2C01    // Must have "load IA32_PAT" bit set
#define VMCS_HOST_IA32_EFER_FULL                    0x2C02    // Must have "load IA32_EFER" bit set
#define VMCS_HOST_IA32_EFER_HIGH                    0x2C03    // Must have "load IA32_EFER" bit set
#define VMCS_HOST_IA32_PERF_GLB_CTRL_FULL           0x2C04    // Must have "load IA32_PERF_GLOBAL_CTRL" bit set
#define VMCS_HOST_IA32_PERF_GLB_CTRL_HIGH           0x2C05    // Must have "load IA32_PERF_GLOBAL_CTRL" bit set
#define VMCS_HOST_IA32_PKRS_FULL                    0x2C06    // Must have "load PKRS" bit set
#define VMCS_HOST_IA32_PKRS_HIGH                    0x2C07    // Must have "load PKRS" bit set


/* 32-Bit Control Fields (B.3.1) */
#define VMCS_CTRL_PIN_EXEC_CTRLS                    0x4000
#define VMCS_CTRL_PRIMARY_EXEC_CTRLS                0x4002
#define VMCS_CTRL_EXCEPT_BITMAP                     0x4004
#define VMCS_CTRL_PAGE_FAULT_ERR_MASK               0x4006
#define VMCS_CTRL_PAGE_FAULT_ERR_MATCH              0x4008
#define VMCS_CTRL_CR3_TARGET_COUNT                  0x400A
#define VMCS_CTRL_VM_EXIT_CTRLS                     0x400C
#define VMCS_CTRL_VM_EXIT_MSR_STORE_COUNT           0x400E
#define VMCS_CTRL_VM_EXIT_MSR_LOAD_COUNT            0x4010
#define VMCS_CTRL_VM_ENTRY_CTRLS                    0x4012
#define VMCS_CTRL_VM_ENTRY_MSR_LOAD_COUNT           0x4014
#define VMCS_CTRL_VM_ENTRY_INT_INFO_FIELD           0x4016
#define VMCS_CTRL_VM_ENTRY_EXCEPT_ERR_CODE          0x4018
#define VMCS_CTRL_VM_ENTRY_INSTR_LEN                0x401A
#define VMCS_CTRL_TPR_THRESHOLD                     0x401C    // Must have "use TPR shadow" bit set
#define VMCS_CTRL_SECONDARY_EXEC_CTRLS              0x401E    // Must have "activate secondary controls" bit set
#define VMCS_CTRL_PLE_GAP                           0x4020    // Must have "PAUSE-loop exiting" bit set
#define VMCS_CTRL_PLE_WINDOW                        0x4022    // Must have "PAUSE-loop exiting" bit set


/* 32-Bit Read-Only Data Fields (B.3.2) */
#define VMCS_RO_VM_INSTR_ERR                        0x4400
#define VMCS_RO_EXIT_REASON                         0x4402
#define VMCS_RO_VM_EXIT_INT_INFO                    0x4404
#define VMCS_RO_VM_EXIT_INT_ERR_CODE                0x4406
#define VMCS_RO_IDT_VEC_INFO_FIELD                  0x4408
#define VMCS_RO_IDT_VEC_ERR_CODE                    0x440A
#define VMCS_RO_VM_EXIT_INSTR_LEN                   0x440C
#define VMCS_RO_VM_EXIT_INSTR_INFO                  0x440E


/* 32-Bit Guest-State Fields (B.3.3) */
#define VMCS_GUEST_ES_LIMIT                         0x4800
#define VMCS_GUEST_CS_LIMIT                         0x4802
#define VMCS_GUEST_SS_LIMIT                         0x4804
#define VMCS_GUEST_DS_LIMIT                         0x4806
#define VMCS_GUEST_FS_LIMIT                         0x4808
#define VMCS_GUEST_GS_LIMIT                         0x480A
#define VMCS_GUEST_LDTR_LIMIT                       0x480C
#define VMCS_GUEST_TR_LIMIT                         0x480E
#define VMCS_GUEST_GDTR_LIMIT                       0x4810
#define VMCS_GUEST_IDTR_LIMIT                       0x4812
#define VMCS_GUEST_ES_ACCESS_RIGHTS                 0x4814
#define VMCS_GUEST_CS_ACCESS_RIGHTS                 0x4816
#define VMCS_GUEST_SS_ACCESS_RIGHTS                 0x4818
#define VMCS_GUEST_DS_ACCESS_RIGHTS                 0x481A
#define VMCS_GUEST_FS_ACCESS_RIGHTS                 0x481C
#define VMCS_GUEST_GS_ACCESS_RIGHTS                 0x481E
#define VMCS_GUEST_LDTR_ACCESS_RIGHTS               0x4820
#define VMCS_GUEST_TR_ACCESS_RIGHTS                 0x4822
#define VMCS_GUEST_INT_STATE                        0x4824
#define VMCS_GUEST_ACTIVITY_STATE                   0x4826
#define VMCS_GUEST_SMBASE                           0x4828
#define VMCS_GUEST_IA32_SYSENTER_CS                 0x482A
#define VMCS_GUEST_VMX_PREEMP_TIMER_VAL             0x482E    // Must have "activate VMX-preemption timer" bit set


/* 32-Bit Host-State Field (B.3.4) */
#define VMCS_HOST_IA32_SYSENTER_CS                  0x4C00


/* Natural-Width Control Fields (B.4.1) */
#define VMCS_CTRL_CR0_GUEST_HOST_MASK               0x6000
#define VMCS_CTRL_CR4_GUEST_HOST_MASK               0x6002
#define VMCS_CTRL_CR0_READ_SHADOW                   0x6004
#define VMCS_CTRL_CR4_READ_SHADOW                   0x6006
#define VMCS_CTRL_CR3_TARGET_VAL_0                  0x6008
#define VMCS_CTRL_CR3_TARGET_VAL_1                  0x600A
#define VMCS_CTRL_CR3_TARGET_VAL_2                  0x600C
#define VMCS_CTRL_CR3_TARGET_VAL_3                  0x600E


/* Natural-Width Read-Only Data Fields (B.4.2) */
#define VMCS_RO_EXIT_QUAL                           0x6400
#define VMCS_RO_IO_RCX                              0x6402
#define VMCS_RO_IO_RSI                              0x6404
#define VMCS_RO_IO_RDI                              0x6406
#define VMCS_RO_IO_RIP                              0x6408
#define VMCS_RO_GUEST_LIN_ADDR                      0x640A


/* Natural-Width Guest-State Fields (B.4.3) */
#define VMCS_GUEST_CR0                              0x6800
#define VMCS_GUEST_CR3                              0x6802
#define VMCS_GUEST_CR4                              0x6804
#define VMCS_GUEST_ES_BASE                          0x6806
#define VMCS_GUEST_CS_BASE                          0x6808
#define VMCS_GUEST_SS_BASE                          0x680A
#define VMCS_GUEST_DS_BASE                          0x680C
#define VMCS_GUEST_FS_BASE                          0x680E
#define VMCS_GUEST_GS_BASE                          0x6810
#define VMCS_GUEST_LDTR_BASE                        0x6812
#define VMCS_GUEST_TR_BASE                          0x6814
#define VMCS_GUEST_GDTR_BASE                        0x6816
#define VMCS_GUEST_IDTR_BASE                        0x6818
#define VMCS_GUEST_DR7                              0x681A
#define VMCS_GUEST_RSP                              0x681C
#define VMCS_GUEST_RIP                              0x681E
#define VMCS_GUEST_RFLAGS                           0x6820
#define VMCS_GUEST_PENDING_DBG_EXCEPTS              0x6822
#define VMCS_GUEST_IA32_SYSENTER_ESP                0x6824
#define VMCS_GUEST_IA32_SYSENTER_EIP                0x6826
#define VMCS_GUEST_IA32_S_CET                       0x6828
#define VMCS_GUEST_SSP                              0x682A
#define VMCS_GUEST_IA32_INTERRUPT_SSP_TABLE_ADDR    0x682C


/* Natural-Width Host-State Fields (B.4.4) */
#define VMCS_HOST_CR0                               0x6C00
#define VMCS_HOST_CR3                               0x6C02
#define VMCS_HOST_CR4                               0x6C04
#define VMCS_HOST_FS_BASE                           0x6C06
#define VMCS_HOST_GS_BASE                           0x6C08
#define VMCS_HOST_TR_BASE                           0x6C0A
#define VMCS_HOST_GDTR_BASE                         0x6C0C
#define VMCS_HOST_IDTR_BASE                         0x6C0E
#define VMCS_HOST_IA32_SYSENTER_ESP                 0x6C10
#define VMCS_HOST_IA32_SYSENTER_EIP                 0x6C12
#define VMCS_HOST_RSP                               0x6C14
#define VMCS_HOST_RIP                               0x6C16
#define VMCS_HOST_IA32_S_CET                        0x6C18
#define VMCS_HOST_SSP                               0x6C1A
#define VMCS_HOST_IA32_INTERRUPT_SSP_TABLE_ADDR     0x6C1C

#pragma warning(push)

#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int

// [24.11.5] "VMXON Region"
typedef struct _VMXON_REGION
{
    UINT32 RevisionIdentifier : 31;
    UINT32 Reserved0 : 1;
} VMXON_REGION, *PVMXON_REGION;

// [24.2] "Format of the VMCS Region"
typedef struct _VMCS_REGION
{
    UINT32 RevisionIdentifier : 31;
    UINT32 ShadowIndicator : 1;
    UINT32 AbortIndicator;
    // ...
} VMCS, *PVMCS;

// volume 3C, section 24.6 "VM-Execution Control Fields"

// [24.6.1] "Pin-Based VM-Execution Controls", Table 24-5
typedef union _PIN_VM_EXEC_CTRLS
{
    struct
    {
        UINT32 ExternalInterruptExiting : 1;        // 0
        UINT32 Reserved0 : 2;                       // 1-2
        UINT32 NMIExiting : 1;                      // 3
        UINT32 Reserved1 : 1;                       // 4
        UINT32 VirtualNMIs : 1;                     // 5
        UINT32 ActivateVMXPreemptionTimer : 1;      // 6
        UINT32 ProcessPostedInterrupts : 1;         // 7
    };
    UINT32 All;
} PIN_VM_EXEC_CTRLS;

// [24.6.2] "Processor-Based VM-Execution Controls", Table 24-6
typedef union _PROCESSOR_PRIMARY_VM_EXEC_CTRLS
{
    struct
    {
        UINT32 Reserved0 : 2;                       // 0-1
        UINT32 InterruptWindowExiting : 1;          // 2
        UINT32 TSCOffsetting : 1;                   // 3
        UINT32 Reserved1 : 3;                       // 4-6
        UINT32 HLTExiting : 1;                      // 7
        UINT32 Reserved2 : 1;                       // 8
        UINT32 INVLPGExiting : 1;                   // 9
        UINT32 MWAITExiting : 1;                    // 10
        UINT32 RDPMCExiting : 1;                    // 11
        UINT32 RDTSCExiting : 1;                    // 12
        UINT32 Reserved3 : 2;                       // 13-14
        UINT32 CR3LoadExiting : 1;                  // 15
        UINT32 CR3StoreExiting : 1;                 // 16
        UINT32 Reserved4 : 2;                       // 17-18
        UINT32 CR8LoadExiting : 1;                  // 19
        UINT32 CR8StoreExiting : 1;                 // 20
        UINT32 UseTRPShadow : 1;                    // 21
        UINT32 NMIWindowExiting : 1;                // 22
        UINT32 MOVDRExiting : 1;                    // 23
        UINT32 UnconditionalIOExiting : 1;          // 24
        UINT32 UseIOBitmaps : 1;                    // 25
        UINT32 Reserved5 : 1;                       // 26
        UINT32 MonitorTrapFlag : 1;                 // 27
        UINT32 UseMSRBitmaps : 1;                   // 28
        UINT32 MONITORExiting : 1;                  // 29
        UINT32 PAUSEExiting : 1;                    // 30
        UINT32 ActivateSecondaryControls : 1;       // 31
    };
    UINT32 All;
} PROCESSOR_PRIMARY_VM_EXEC_CTRLS;

// [24.6.2] "Processor-Based VM-Execution Controls", Table 24-7
typedef union _PROCESSOR_SECONDARY_VM_EXEC_CTRLS
{
    struct
    {
        UINT32 VirtualizeAPICAccess : 1;            // 0
        UINT32 EnableEPT : 1;                       // 1
        UINT32 DescriptorTableExiting : 1;          // 2
        UINT32 EnableRDTSCP : 1;                    // 3
        UINT32 VirtualizeX2APICMode : 1;            // 4
        UINT32 EnableVPID : 1;                      // 5
        UINT32 WBINVDExiting : 1;                   // 6
        UINT32 UnrestrictedGuest : 1;               // 7
        UINT32 VirtualizeAPICReg : 1;               // 8
        UINT32 VirtualInterruptDelivery : 1;        // 9
        UINT32 PAUSELoopExiting : 1;                // 10
        UINT32 RDRANDExiting : 1;                   // 11
        UINT32 EnableINVPCID : 1;                   // 12
        UINT32 EnableVMFUNC : 1;                    // 13
        UINT32 VMCSShadowing : 1;                   // 14
        UINT32 ENCLSExiting : 1;                    // 15
        UINT32 RDSEEDExiting : 1;                   // 16
        UINT32 EnablePML : 1;                       // 17
        UINT32 EPTViolationVirtExcept : 1;          // 18
        UINT32 HideVMXNonRootFromIPT : 1;           // 19
        UINT32 EnableXSAVESXRSTORS : 1;             // 20
        UINT32 Reserved0 : 1;                       // 21
        UINT32 ModeBasedEPTExecuteCtrl : 1;         // 22
        UINT32 Reserved1 : 2;                       // 23-24
        UINT32 UseTSCScaling : 1;                   // 25
    };
    UINT32 All;
} PROCESSOR_SECONDARY_VM_EXEC_CTRLS;

// [24.7.1] "VM-Exit Controls", Table 24-10
typedef union _VM_EXIT_CTRLS
{
    struct
    {
        UINT32 Reserved0 : 2;                       // 0-1
        UINT32 SaveDebugControls : 1;               // 2
        UINT32 Reserved1 : 6;                       // 3-8
        UINT32 HostAddressSpaceSize : 1;            // 9
        UINT32 Reserved2 : 2;                       // 10-11
        UINT32 LoadPerfGlobalCtrl : 1;              // 12        // MSR
        UINT32 Reserved3 : 2;                       // 13-14
        UINT32 AcknowledgeInterruptOnExit : 1;      // 15
        UINT32 Reserved4 : 2;                       // 16-17
        UINT32 SavePAT : 1;                         // 18        // MSR
        UINT32 LoadPAT : 1;                         // 19        // MSR
        UINT32 SaveEFER : 1;                        // 20        // MSR
        UINT32 LoadEFER : 1;                        // 21        // MSR
        UINT32 SaveVMXPreemptionTimer : 1;          // 22
        UINT32 ClearBNDCFGS : 1;                    // 23        // MSR
        UINT32 HideVMExitsFromPT : 1;               // 24
    };
    UINT32 All;
} VM_EXIT_CTRLS;

// [24.8.1] "VM-Entry Controls", Table 24-12
typedef union _VM_ENTRY_CTRLS
{
    struct
    {
        UINT32 Reserved0 : 2;                       // 0-1
        UINT32 LoadDebugControls : 1;               // 2
        UINT32 Reserved1 : 6;                       // 3-8
        UINT32 IA32eModeGuest : 1;                  // 9
        UINT32 EntryToSMM : 1;                      // 10
        UINT32 DisableDualMonTreatment : 1;         // 11
        UINT32 Reserved2 : 1;                       // 12
        UINT32 LoadPerfGlobalCtrl : 1;              // 13        // MSR
        UINT32 LoadPAT : 1;                         // 14
        UINT32 LoadEFER : 1;                        // 15
        UINT32 LoadBNDCFGS : 1;                     // 16
        UINT32 HideVMEntriesFromPT : 1;             // 17
    };
    UINT32 All;
} VM_ENTRY_CTRLS;

// [] ""
typedef union _VM_EXIT_REASON
{
    struct
    {
        UINT32 BasicReason : 16;                    // 0-15
        UINT32 Reserved0 : 11;                      // 16-26
        UINT32 InEnclave : 1;                       // 27
        UINT32 PendingMFT : 1;                      // 28
        UINT32 RootExit : 1;                        // 29
        UINT32 Reserved1 : 1;                       // 30
        UINT32 EntryFailure : 1;                    // 31
    };
    UINT32 All;
} VM_EXIT_REASON;

#pragma warning(pop)

#endif // __VMCS_H__
