#ifndef CPU_H
#define CPU_H
#include "comm/types.h"


#pragma pack(1)
/**
 * GDT描述符/段描述符
 * Figure 3-8. Segment Descriptor
 */
typedef struct _segment_desc_t {
	uint16_t limit15_0;
	uint16_t base15_0;
	uint8_t base23_16;
	uint16_t attr;
	uint8_t base31_24;
}segment_desc_t;


/*
Figure 6-2. IDT Gate Descriptors
*/
typedef struct _gate_desc_t {
	uint16_t offset15_0;
	uint16_t selector;
	uint16_t attr;
	uint16_t offset31_16;
}gate_desc_t;



# pragma pack()

// Determines the scaling of the segment limit field. When the granularity flag is clear, the segment
// limit is interpreted in byte units; when flag is set, the segment limit is interpreted in 4-KByte units.
#define SEG_G (1 << 15) // 第15位 attr中的G

#define SEG_D (1 << 14) // 第14位 attr中的D

// Indicates whether the segment is present in memory (set) or not present (clear).
#define SEG_P_PRESENT (1 << 7)

#define SEG_DPL0 (0 << 5)
#define SEG_DPL3 (3 << 5)

#define SEG_S_SYSTEM (0 << 4)
#define SEG_S_NORMAL (1 << 4)

#define SEG_TYPE_CODE (1 << 3)
#define SEG_TYPE_DATA (0 << 3)

#define SEG_TYPE_RW (1 << 1)

#define GATE_TYPE_IDT		(0xE << 8)		// 中断32位门描述符
#define GATE_P_PRESENT		(1 << 15)		// 是否存在
#define GATE_DPL0			(0 << 13)		// 特权级0，最高特权级
#define GATE_DPL3			(3 << 13)		// 特权级3，最低权限

void cpu_init (void);
void segment_desc_set (int selector, uint32_t base, uint32_t limit, uint16_t attr);
void gate_desc_set (gate_desc_t* desc, uint16_t selector, uint32_t offset, uint16_t attr);
#endif