#ifndef __SEG_H__
#define __SEG_H__

#include <wdm.h>

#define SELECTOR_INDEX_MASK					0xFFF8

#define DESCRIPTOR_TYPE_SYSTEM				0
#define DESCRIPTOR_TYPE_CODE_DATA			1

#define SYS_SEG_DESC_TYPE_LDT				2
#define SYS_SEG_DESC_TYPE_TSS_BUSY			11
#define SYS_SEG_DESC_TYPE_TSS_AVAILABLE		9

#pragma warning(push)

#pragma warning(disable:4201) // nonstandard extension used: nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used: bit field types other than int

// [3.4.2] "Segment Selectors", Figure 3-6
typedef union _SEGMENT_SELECTOR
{
	struct {
		UINT16 RPL : 2;					// 0-1
		UINT16 TI : 1;					// 2
		UINT16 Index : 13;				// 3-15
	};
	UINT16 All;
} SEG_SEL, *PSEG_SEL;

// [3.4.5] "Segment Descriptors", Figure 3-8
typedef union _SEGMENT_DESCRIPTOR
{
	struct {
		UINT64 Limit : 16;				// 0-15
		UINT64 Base : 24;				// 16-39
		UINT64 Type : 4;				// 40-43
		UINT64 DescType : 1;			// 44
		UINT64 DPL : 2;					// 45-46
		UINT64 Present : 1;				// 47
		UINT64 Limit2 : 4;				// 48-51
		UINT64 Available : 1;			// 52
		UINT64 LongModeCS : 1;			// 53
		UINT64 DefOpSize : 1;			// 54
		UINT64 Granularity : 1;			// 55
		UINT64 Base2 : 8;				// 56-63
	};
	UINT64 All;
} SEG_DESC, *PSEG_DESC;

// [7.2.3] "TSS Descriptor in 64-bit Mode", Figure 7-4
// Note: only for usage in TSS or LDT system segment descriptors
typedef struct _SYSTEM_SEGMENT_DESCRIPTOR
{
	SEG_DESC LowerDesc;
	UINT32 Base3;
	UINT32 Reserved0;
} SYS_SEG_DESC, *PSYS_SEG_DESC;

// [24.4.1] "Guest Register State", Table 24-2
typedef union _SEGMENT_ACCESS_RIGHTS
{
	struct {
		UINT32 SegType : 4;				// 0-3
		UINT32 DescType : 1;			// 4
		UINT32 DPL : 2;					// 5-6
		UINT32 Present : 1;				// 7
		UINT32 Reserved0 : 4;			// 8-11
		UINT32 Available : 1;			// 12
		UINT32 LongModeCS : 1;			// 13
		UINT32 DefOpSize : 1;			// 14
		UINT32 Granularity : 1;			// 15
		UINT32 Unusable : 1;			// 16
		UINT32 Reserved1 : 15;			// 17-31
	};
	UINT32 All;
} SEG_ACCESS_RIGHTS, *PSEG_ACCESS_RIGHTS;

// [2.4] "Memory Management Registers", Figure 2-6
//		Used for the GDTR & IDTR
#pragma pack(push, 1)
typedef struct _SYSTEM_TABLE_REGISTER
{
	UINT16 Limit;
	UINT64 Base;
} SYSTEM_TABLE_REGISTER, *PSYSTEM_TABLE_REGISTER;
#pragma pack(pop)

#pragma warning(pop)

// See the implementation in "./asm/Seg.asm"
extern UINT16 __readcs();
extern UINT16 __readss();
extern UINT16 __readds();
extern UINT16 __reades();
extern UINT16 __readfs();
extern UINT16 __readgs();
extern UINT16 __readldtr();
extern UINT16 __readtr();

extern void __sgdt(PSYSTEM_TABLE_REGISTER pGDTR);
extern void __lgdt(PSYSTEM_TABLE_REGISTER pGDTR);

extern SEG_ACCESS_RIGHTS __readar(UINT16 /*SEG_SEL*/ segment);

SEG_ACCESS_RIGHTS
ReadAR(
	UINT16 selector
	);

// Seg.c
UINT64
__segmentbase(
	UINT16 selector
	);

#endif // __SEG_H__
