#ifndef CPU_H
#define CPU_H
#include "comm/types.h"

#define EFLAGS_DEFAULT (1<<1)
#define EFLAGS_IF (1<<9)
#pragma pack(1)
/**
 * GDT描述符/段描述符/TSS描述符
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

/**
 * tss Figure 8-2. 32-Bit Task-State Segment (TSS)
 */
typedef struct _tss_t {
    uint32_t pre_link;
    uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint32_t iomap;
}tss_t;

# pragma pack()

// Determines the scaling of the segment limit field. When the granularity flag is clear, the segment
// limit is interpreted in byte units; when flag is set, the segment limit is interpreted in 4-KByte units.
#define SEG_G (1 << 15) // 第15位 attr中的G

#define SEG_D (1 << 14) // 第14位 attr中的D

// Indicates whether the segment is present in memory (set) or not present (clear).
#define SEG_P_PRESENT (1 << 7)

// SEG_DPL0表示最高特权级别（通常是内核模式），而SEG_DPL3表示最低特权级别（通常是用户模式）
#define SEG_DPL0 (0 << 5)
#define SEG_DPL3 (3 << 5)

#define SEG_RPL0                (0 << 0)
#define SEG_RPL3                (3 << 0)

#define SEG_CPL0                (0 << 0)
#define SEG_CPL3                (3 << 0)

#define SEG_S_SYSTEM (0 << 4)
#define SEG_S_NORMAL (1 << 4)

#define SEG_TYPE_CODE (1 << 3)
#define SEG_TYPE_DATA (0 << 3)
#define SEG_TYPE_TSS  (9 << 0)

#define SEG_TYPE_RW (1 << 1)

#define GATE_TYPE_IDT		(0xE << 8)		// 中断32位门描述符
#define GATE_P_PRESENT		(1 << 15)		// 是否存在
#define GATE_DPL0			(0 << 13)		// 特权级0，最高特权级
#define GATE_DPL3			(3 << 13)		// 特权级3，最低权限

void cpu_init (void);
void segment_desc_set (int selector, uint32_t base, uint32_t limit, uint16_t attr);
void gate_desc_set (gate_desc_t* desc, uint16_t selector, uint32_t offset, uint16_t attr);
// 从gdt表中分配一个空的描述符位置，从1开始
int gdt_alloc_desc();
void gdt_free_sel(int sel);
void switch_to_tss (int tss_sel);
#endif