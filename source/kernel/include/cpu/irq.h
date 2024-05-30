#ifndef IRQ_H
#define IRQ_H
# include "comm/types.h"

// 中断号码
#define IRQ0_DE             0 //除0中断

typedef struct _exception_frame_t {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t num, error_code;
    uint32_t eip, cs, eflags;
} _exception_frame_t;

typedef void (*irq_handler_t) (void);

void irq_init(void);
void irq_install(int num, irq_handler_t handler);

#endif 