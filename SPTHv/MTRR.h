#ifndef __MTRR_H__
#define __MTRR_H__

#include "MSR.h"
#include "Memory.h"

#include <intrin.h>



// [Table 11-9] "Address Mapping for Fixed-Range MTRRs"
//  Fixed range MTRRs occupy the first megabyte of memory (00000-FFFFF)
#define FIXED_MTRR_RANGE_LOW                0x00000
#define FIXED_MTRR_RANGE_HIGH               0xFFFFF

#define MTRR_FIXED_RANGE_REG_COUNT          11
#define MTRR_FIXED_RANGE_SUB_SET_COUNT      8

#define GET_VAR_MTRR_PHYSBASE(vidx) ( __readmsr(IA32_MTRR_PHYSBASE0 + (vidx * 2)) )
#define GET_VAR_MTRR_PHYSMASK(vidx) ( __readmsr(IA32_MTRR_PHYSMASK0 + (vidx * 2)) )



//
// Local structure definitions
//

// [11.11] Memory Type Range Registers (MTRRs)

// [Table 11-8] "Memory Types That Can Be Encoded in MTRRs"

typedef enum _MTRR_MEM_TYPE
{
    MTRR_MEM_UC,
    MTRR_MEM_WC,
    MTRR_MEM_WT = 4,
    MTRR_MEM_WP,
    MTRR_MEM_WB,
    MTRR_MEM_UNKNOWN
} MTRR_MEM_TYPE;

#pragma warning(push)

#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int
#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union

// [Figure 11-5] "IA32_MTRRCAP Register"
typedef union _MTRR_CAPABILITIES
{
    struct
    {
        UINT64 VariableCount : 8;                   // 0-7
        UINT64 FixedSupport : 1;                    // 8
        UINT64 Reserved0 : 1;                       // 9
        UINT64 MemWCSupport : 1;                    // 10
        UINT64 SMRRSupport : 1;                     // 11
        UINT64 PRMRRSupport : 1;                    // 12
    };
    UINT64 All;
} MTRR_CAP;

// [Figure 11-6] "IA32_MTRR_DEF_TYPE MSR"
typedef union _MTRR_DEFAULT_TYPE_INFORMATION
{
    struct
    {
        UINT64 MemType : 8;                         // 0-7
        UINT64 Reserved0 : 2;                       // 8-9
        UINT64 FixedSupport : 1;                    // 10
        UINT64 MTRRSupport : 1;                     // 11
    };
    UINT64 All;
} MTRR_DEFS;

// [Figure 11-7] "IA32_MTRR_PHYSBASEn and IA32_MTRR_PHYSMASKn Variable-Range Register Pair"
typedef union _MTRR_PHYS_BASE
{
    struct
    {
        UINT64 MemType : 8;                         // 0-7
        UINT64 Reserved0 : 4;                       // 8-11
        UINT64 PhysBase : 28;                       // 12-40
        UINT64 Reserved1 : 24;                      // 41-63
    };
    UINT64 All;
} MTRR_PHYS_BASE;

// " "
typedef union _MTRR_PHYS_MASK
{
    struct
    {
        UINT64 Reserved0 : 11;                      // 0-10
        UINT64 Valid : 1;                           // 11
        UINT64 PhysMask : 28;                       // 12-40
        UINT64 Reserved1 : 24;                      // 41-63
    };
    UINT64 All;
} MTRR_PHYS_MASK;

#pragma warning(pop)

typedef struct __FIXED_RANGE_ENTRY
{
    ULONG MaskMSR;
    UINT64 StartAddress;
    UINT64 EndAddress;
    UINT64 SubRangeSize;
} FIXED_RANGE_ENTRY;



//
// Global definitions
//

// [Table 11-9] "Address Mapping for Fixed-Range MTRRs"
static CONST FIXED_RANGE_ENTRY g_FixedRanges[] = {
    { IA32_MTRR_FIX64K_00000, 0x00000, 0x7FFFF, 0x10000 },
    { IA32_MTRR_FIX16K_80000, 0x80000, 0x9FFFF, 0x4000  },
    { IA32_MTRR_FIX16K_A0000, 0xA0000, 0xBFFFF, 0x4000  },
    { IA32_MTRR_FIX4K_C0000,  0xC0000, 0xC7FFF, 0x1000  },
    { IA32_MTRR_FIX4K_C8000,  0xC8000, 0xCFFFF, 0x1000  },
    { IA32_MTRR_FIX4K_D0000,  0xD0000, 0xD7FFF, 0x1000  },
    { IA32_MTRR_FIX4K_D8000,  0xD8000, 0xDFFFF, 0x1000  },
    { IA32_MTRR_FIX4K_E0000,  0xE0000, 0xE7FFF, 0x1000  },
    { IA32_MTRR_FIX4K_E8000,  0xE8000, 0xEFFFF, 0x1000  },
    { IA32_MTRR_FIX4K_F0000,  0xF0000, 0xF7FFF, 0x1000  },
    { IA32_MTRR_FIX4K_F8000,  0xF8000, 0xFFFFF, 0x1000  }
};

C_ASSERT( sizeof(g_FixedRanges) / sizeof(FIXED_RANGE_ENTRY) == MTRR_FIXED_RANGE_REG_COUNT );



//
// Local function definitions
//

BOOLEAN
CheckMTTRSupport();

VOID
MtrrGetMemType(
    _In_ CONST UINT64 RangeStart,
    _In_ CONST SIZE_T RangeSize,
    _Inout_ MTRR_MEM_TYPE* CONST MemType
    );

#endif // __MTRR_H__
