#ifndef __CPU_H__
#define __CPU_H__

#include <wdm.h>

#define CPUID_FEATURE_INFORMATION           1
#define CPUID_ADDRESS_WIDTHS                0x80000008

#define CPUID_FEATURE_MTRR_ENABLED          0x1000
#define CPUID_PHYS_ADDR_WIDTH               0xFF

typedef struct _CPUID_INFORMATION
{
    int EAX;
    int EBX;
    int ECX;
    int EDX;
} CPUID_INFO;

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

#endif // __CPU_H__
