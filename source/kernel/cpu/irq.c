#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"
#define IDT_TABLE_NR 128

void exception_handler_unknow(void);
void exception_handler_division(void);

static void do_default_handler (const char* message) {
    for (;;) {}
}

void do_handler_unknow (_exception_frame_t* frame) {
    do_default_handler("unknow exception");
}


void do_handler_division (_exception_frame_t* frame) {
    do_default_handler("division exception");
}

static gate_desc_t idt_table[IDT_TABLE_NR];


void irq_init(void) {
    for (int i=0; i<IDT_TABLE_NR; i++) {
        gate_desc_set(idt_table+i, KERNEL_SELECTOR_CS, (uint32_t)exception_handler_unknow, 
        GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_IDT);
    }

    irq_install(IRQ0_DE, (irq_handler_t)exception_handler_division);

    lidt((uint32_t)idt_table, sizeof(idt_table));
}

void irq_install(int irq_num, irq_handler_t handler) {
    if (irq_num >= IDT_TABLE_NR) {
        return -1;
    }

    gate_desc_set(idt_table+irq_num, KERNEL_SELECTOR_CS, (uint32_t)handler, 
        GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_IDT);
    return 0;
}
