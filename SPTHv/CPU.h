#ifndef __CPU_H__
#define __CPU_H__

#include <wdm.h>

#pragma warning(push)

#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int

// [2.5] "Control Registers" (Figure 2-7)
typedef union _CR0
{
    struct
    {
        UINT64 ProtectionEnable : 1;                // 0
        UINT64 MonitorCoprocessor : 1;              // 1
        UINT64 Emulation : 1;                       // 2
        UINT64 TaskSwitched : 1;                    // 3
        UINT64 ExtensionType : 1;                   // 4
        UINT64 NumericError : 1;                    // 5
        UINT64 Reserved0 : 10;                      // 6-15
        UINT64 WriteProtect : 1;                    // 16
        UINT64 Reserved1 : 1;                       // 17
        UINT64 AlignmentMask : 1;                   // 18
        UINT64 Reserved2 : 10;                      // 19-28
        UINT64 NotWriteThrough : 1;                 // 29
        UINT64 CacheDisabled : 1;                   // 30
        UINT64 Paging : 1;                          // 31
    };
    struct
    {
        UINT64 PE : 1;                              // 0
        UINT64 MP : 1;                              // 1
        UINT64 EM : 1;                              // 2
        UINT64 TS : 1;                              // 3
        UINT64 ET : 1;                              // 4
        UINT64 NE : 1;                              // 5
        UINT64 _Reserved0 : 10;                     // 6-15
        UINT64 WP : 1;                              // 16
        UINT64 _Reserved1 : 1;                      // 17
        UINT64 AM : 1;                              // 18
        UINT64 _Reserved2 : 10;                     // 19-28
        UINT64 NW : 1;                              // 29
        UINT64 CD : 1;                              // 30
        UINT64 PG: 1;                               // 31
    };
    UINT64 All;
} CR0;

// [2.5] "Control Registers" (Figure 2-7)
typedef union _CR4
{
    struct
    {
        UINT64 Virtual8086ModeExt : 1;              // 0
        UINT64 ProtectedModeVirtInt : 1;            // 1
        UINT64 TimeStampDisable : 1;                // 2
        UINT64 DebuggingExt : 1;                    // 3
        UINT64 PageSizeExt : 1;                     // 4
        UINT64 PhysicalAddressExt : 1;              // 5
        UINT64 MachineCheckEnable : 1;              // 6
        UINT64 PageGlobalEnable : 1;                // 7
        UINT64 PerformanceMonCounterEnable : 1;     // 8
        UINT64 FXSAVEFXRSTORSupport : 1;            // 9
        UINT64 UnmaskedSIMDExceptSupport : 1;       // 10
        UINT64 UsermodeInstructPrevention : 1;      // 11
        UINT64 LinearAddresses57Bit : 1;            // 12
        UINT64 VMXEnable : 1;                       // 13
        UINT64 SMXEnable : 1;                       // 14
        UINT64 Reserved0 : 1;                       // 15
        UINT64 FSGSBASEEnable : 1;                  // 16
        UINT64 PCIDEnable : 1;                      // 17
        UINT64 XSAVEEnable : 1;                     // 18
        UINT64 Reserved1 : 1;                       // 19
        UINT64 SMEPEnable : 1;                      // 20
        UINT64 SMAPEnable : 1;                      // 21
        UINT64 UsermodeProtectionKeysEnable : 1;    // 22
        UINT64 ControlFlowEnforcement : 1;          // 23
        UINT64 SupervisorProtectionKeysEnable : 1;  // 24
        // ...
    };
    struct
    {
        UINT64 VME : 1;                             // 0
        UINT64 PVI : 1;                             // 1
        UINT64 TSD : 1;                             // 2
        UINT64 DE : 1;                              // 3
        UINT64 PSE : 1;                             // 4
        UINT64 PAE : 1;                             // 5
        UINT64 MCE : 1;                             // 6
        UINT64 PGE : 1;                             // 7
        UINT64 PCE : 1;                             // 8
        UINT64 OSFXSR : 1;                          // 9
        UINT64 OSXMMEXCPT : 1;                      // 10
        UINT64 UMIP : 1;                            // 11
        UINT64 LA57 : 1;                            // 12
        UINT64 VMXE : 1;                            // 13
        UINT64 SMXE : 1;                            // 14
        UINT64 _Reserved0 : 1;                      // 15
        UINT64 FSGSBASE : 1;                        // 16
        UINT64 PCIDE : 1;                           // 17
        UINT64 OSXSAVE : 1;                         // 18
        UINT64 _Reserved1 : 1;                      // 19
        UINT64 SMEP : 1;                            // 20
        UINT64 SMAP : 1;                            // 21
        UINT64 PKE : 1;                             // 22
        UINT64 CET : 1;                             // 23
        UINT64 PKS : 1;                             // 24
        // ...
    };
    UINT64 All;
} CR4;

#pragma warning(pop)

// [6.15] "Exception and Interrupt Reference"
typedef enum _INTERRUPT_VECTOR
{
    // Citations to each of the following can be found within
    //  volume 3A of the Intel SDM under the parent chapter

    INT_VEC_DE,         // [6-24] "Interrupt 0 — Divide Error Exception (#DE)"
    INT_VEC_DB,         // [6-25] "Interrupt 1 — Debug Exception (#DB)"
    INT_VEC_NMI,        // [6-27] "Interrupt 2 — NMI Interrupt"
    INT_VEC_BP,         // [6-28] "Interrupt 3 — Breakpoint Exception (#BP)"
    INT_VEC_OF,         // [6-29] "Interrupt 4 — Overflow Exception (#OF)"
    INT_VEC_BR,         // [6-30] "Interrupt 5 — BOUND Range Exceeded Exception (#BR)"
    INT_VEC_UD,         // [6-31] "Interrupt 6 — Invalid Opcode Exception (#UD)"
    INT_VEC_NM,         // [6-32] "Interrupt 7 — Device Not Available Exception (#NM)"
    INT_VEC_DF,         // [6-33] "Interrupt 8 — Double Fault Exception (#DF)"
    INT_VEC_CSO,        // [6-35] "Interrupt 9 — Coprocessor Segment Overrun"
    INT_VEC_TS,         // [6-36] "Interrupt 10 — Invalid TSS Exception (#TS)"
    INT_VEC_NP,         // [6-38] "Interrupt 11 — Segment Not Present (#NP)"
    INT_VEC_SS,         // [6-40] "Interrupt 12 — Stack Fault Exception (#SS)"
    INT_VEC_GP,         // [6-41] "Interrupt 13 — General Protetion Exception (#GP)"
    INT_VEC_PF,         // [6-44] "Interrupt 14 — [6-44] "Interrupt 14 — Page-Fault Exception (#PF)"
    INT_VEC_MF = 16,    // [6-47] "Interrupt 16 — x87 FPU Floating-Point Error (#MF)"
    INT_VEC_AC,         // [6-49] "Interrupt 17 — Alignment Check Exception (#AC)"
    INT_VEC_MC,         // [6-51] "Interrupt 18 — Machine-Check Exception (#MC)"
    INT_VEC_XM,         // [6-52] "Interrupt 19 — SIMD Floating-Point Exception (#XM)"
    INT_VEC_VE,         // [6-54] "Interrupt 20 — Virtualization Exception (#VE)"
    INT_VEC_CP          // [6-55] "Interrupt 21 — Control Protection Exception (#CP)"
} INT_VECTOR;

#endif // __CPU_H__
