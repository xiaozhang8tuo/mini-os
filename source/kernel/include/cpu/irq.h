#ifndef IRQ_H
#define IRQ_H
# include "comm/types.h"

// 中断号码  6.3.2
#define IRQ0_DE             0 //除0中断
#define IRQ1_DB             1
#define IRQ2_NMI            2
#define IRQ3_BP             3
#define IRQ4_OF             4
#define IRQ5_BR             5
#define IRQ6_UD             6
#define IRQ7_NM             7
#define IRQ8_DF             8
#define IRQ10_TS            10
#define IRQ11_NP            11
#define IRQ12_SS            12
#define IRQ13_GP            13
#define IRQ14_PF            14
#define IRQ16_MF            16
#define IRQ17_AC            17
#define IRQ18_MC            18
#define IRQ19_XM            19
#define IRQ20_VE            20

#define IRQ0_TIMER          0x20

// PIC控制器相关的寄存器及位配置
#define PIC0_ICW1			0x20
#define PIC0_ICW2			0x21
#define PIC0_ICW3			0x21
#define PIC0_ICW4			0x21
#define PIC0_OCW2			0x20
#define PIC0_IMR			0x21

#define PIC1_ICW1			0xa0
#define PIC1_ICW2			0xa1
#define PIC1_ICW3			0xa1
#define PIC1_ICW4			0xa1
#define PIC1_OCW2			0xa0
#define PIC1_IMR			0xa1

#define PIC_ICW1_ICW4		(1 << 0)		// 1 - 需要初始化ICW4
#define PIC_ICW1_ALWAYS_1	(1 << 4)		// 总为1的位
#define PIC_ICW4_8086	    (1 << 0)        // 8086工作模式

#define IRQ_PIC_START		0x20			// PIC中断起始号
#define PIC_OCW2_EOI (1<<5)


typedef struct exception_frame_t {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t num, error_code;
    uint32_t eip, cs, eflags;
} exception_frame_t;

typedef void (*irq_handler_t) (void);

void irq_init(void);
void irq_install(int num, irq_handler_t handler);

void irq_enable(int irq_num);
void irq_disable(int irq_num);
void irq_disable_global (void);
void irq_enable_global (void);

void pic_send_eoi(int irq_num);
typedef uint32_t irq_state_t;
irq_state_t irq_enter_protection(void);
void irq_leave_protection(irq_state_t state);
#endif 