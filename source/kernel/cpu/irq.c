#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"
#include "tools/log.h"
#define IDT_TABLE_NR 128

void exception_handler_unknow(void);
void exception_handler_division(void);
void exception_handler_Debug (void);
void exception_handler_NMI (void);
void exception_handler_breakpoint (void);
void exception_handler_overflow (void);
void exception_handler_bound_range (void);
void exception_handler_invalid_opcode (void);
void exception_handler_device_unavailable (void);
void exception_handler_double_fault (void);
void exception_handler_invalid_tss (void);
void exception_handler_segment_not_present (void);
void exception_handler_stack_segment_fault (void);
void exception_handler_general_protection (void);
void exception_handler_page_fault (void);
void exception_handler_fpu_error (void);
void exception_handler_alignment_check (void);
void exception_handler_machine_check (void);
void exception_handler_simd_exception (void);
void exception_handler_virtual_exception (void);

void dump_core_regs(exception_frame_t* frame) {
    log_printf("IRQ: %d, error_code: %d", frame->num, frame->error_code);
    log_printf("CS: %d\r\n"
                "DS:%d\r\n"
                "ES:%d\r\n"
                "FS:%d\r\n"
                "GS:%d\r\n"
                , frame->cs, frame->ds, frame->es, frame->fs, frame->gs);
    log_printf( "EAX: 0X%x\r\n"
                "EBX: 0X%x\r\n"
                "EDX: 0X%x\r\n"
                "EDI: 0X%x\r\n"
                "ESI: 0X%x\r\n"
                "EBP: 0X%x\r\n"
                "ESP: 0X%x\r\n"
                , frame->eax, frame->ebx, frame->edx, frame->edi, frame->esi, frame->ebp, frame->esp);
    log_printf("EIP: 0x%x\r\nEFLAGS:0x%x", frame->eip, frame->eflags);
}

static void do_default_handler (exception_frame_t* frame, const char* message) {
    log_printf("------------------------");
    log_printf("IRQ/Exception happend: %s", message);
    dump_core_regs(frame);

    for (;;) {
        hlt();
    }
}

void do_handler_unknow (exception_frame_t* frame) {
    do_default_handler(frame, "unknow exception");
}


void do_handler_division (exception_frame_t* frame) {
    do_default_handler(frame, "division exception");
}

void do_handler_Debug(exception_frame_t * frame) {
	do_default_handler(frame, "Debug Exception");
}

void do_handler_NMI(exception_frame_t * frame) {
	do_default_handler(frame, "NMI Interrupt.");
}

void do_handler_breakpoint(exception_frame_t * frame) {
	do_default_handler(frame, "Breakpoint.");
}

void do_handler_overflow(exception_frame_t * frame) {
	do_default_handler(frame, "Overflow.");
}

void do_handler_bound_range(exception_frame_t * frame) {
	do_default_handler(frame, "BOUND Range Exceeded.");
}

void do_handler_invalid_opcode(exception_frame_t * frame) {
	do_default_handler(frame, "Invalid Opcode.");
}

void do_handler_device_unavailable(exception_frame_t * frame) {
	do_default_handler(frame, "Device Not Available.");
}

void do_handler_double_fault(exception_frame_t * frame) {
	do_default_handler(frame, "Double Fault.");
}

void do_handler_invalid_tss(exception_frame_t * frame) {
	do_default_handler(frame, "Invalid TSS");
}

void do_handler_segment_not_present(exception_frame_t * frame) {
	do_default_handler(frame, "Segment Not Present.");
}

void do_handler_stack_segment_fault(exception_frame_t * frame) {
	do_default_handler(frame, "Stack-Segment Fault.");
}

void do_handler_general_protection(exception_frame_t * frame) {
	do_default_handler(frame, "General Protection.");
}

void do_handler_page_fault(exception_frame_t * frame) {
	do_default_handler(frame, "Page Fault.");
}

void do_handler_fpu_error(exception_frame_t * frame) {
	do_default_handler(frame, "X87 FPU Floating Point Error.");
}

void do_handler_alignment_check(exception_frame_t * frame) {
	do_default_handler(frame, "Alignment Check.");
}

void do_handler_machine_check(exception_frame_t * frame) {
	do_default_handler(frame, "Machine Check.");
}

void do_handler_simd_exception(exception_frame_t * frame) {
	do_default_handler(frame, "SIMD Floating Point Exception.");
}

void do_handler_virtual_exception(exception_frame_t * frame) {
	do_default_handler(frame, "Virtualization Exception.");
}

static gate_desc_t idt_table[IDT_TABLE_NR];


static void init_pic(void) {
    // 主片
    // 边缘触发，级联，需要配置icw4, 8086模式
    outb(PIC0_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
    // 对应的中断号起始序号0x20
    outb(PIC0_ICW2, IRQ_PIC_START);
    // 主片IRQ2有从片
    outb(PIC0_ICW3, 1 << 2);
    // 普通全嵌套、非缓冲、非自动结束、8086模式
    outb(PIC0_ICW4, PIC_ICW4_8086);

    // 从片
    // 边缘触发，级联，需要配置icw4, 8086模式
    outb(PIC1_ICW1, PIC_ICW1_ICW4 | PIC_ICW1_ALWAYS_1);
    // 起始中断序号，要加上8
    outb(PIC1_ICW2, IRQ_PIC_START + 8);
    // 没有从片，连接到主片的IRQ2上
    outb(PIC1_ICW3, 2);
    // 普通全嵌套、非缓冲、非自动结束、8086模式
    outb(PIC1_ICW4, PIC_ICW4_8086);

    // 禁止所有中断, 允许从PIC1传来的中断
    outb(PIC0_IMR, 0xFF & ~(1 << 2));
    outb(PIC1_IMR, 0xFF);
}

void irq_init(void) {
    for (int i=0; i<IDT_TABLE_NR; i++) {
        gate_desc_set(idt_table+i, KERNEL_SELECTOR_CS, (uint32_t)exception_handler_unknow, 
        GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_IDT);
    }

    irq_install(IRQ0_DE, (irq_handler_t)exception_handler_division);
    irq_install(IRQ1_DB, exception_handler_Debug);
	irq_install(IRQ2_NMI, exception_handler_NMI);
	irq_install(IRQ3_BP, exception_handler_breakpoint);
	irq_install(IRQ4_OF, exception_handler_overflow);
	irq_install(IRQ5_BR, exception_handler_bound_range);
	irq_install(IRQ6_UD, exception_handler_invalid_opcode);
	irq_install(IRQ7_NM, exception_handler_device_unavailable);
	irq_install(IRQ8_DF, exception_handler_double_fault);
	irq_install(IRQ10_TS, exception_handler_invalid_tss);
	irq_install(IRQ11_NP, exception_handler_segment_not_present);
	irq_install(IRQ12_SS, exception_handler_stack_segment_fault);
	irq_install(IRQ13_GP, exception_handler_general_protection);
	irq_install(IRQ14_PF, exception_handler_page_fault);
	irq_install(IRQ16_MF, exception_handler_fpu_error);
	irq_install(IRQ17_AC, exception_handler_alignment_check);
	irq_install(IRQ18_MC, exception_handler_machine_check);
	irq_install(IRQ19_XM, exception_handler_simd_exception);
	irq_install(IRQ20_VE, exception_handler_virtual_exception);

    lidt((uint32_t)idt_table, sizeof(idt_table));

    init_pic();
}

void irq_install(int irq_num, irq_handler_t handler) {
    if (irq_num >= IDT_TABLE_NR) {
        return -1;
    }

    gate_desc_set(idt_table+irq_num, KERNEL_SELECTOR_CS, (uint32_t)handler, 
        GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_IDT);
    return 0;
}

void irq_enable(int irq_num) {
    if (irq_num < IRQ_PIC_START) {
        return;
    }

    irq_num -= IRQ_PIC_START;
    if (irq_num < 8) {
        uint8_t mask = inb(PIC0_IMR) & ~(1 << irq_num);
        outb(PIC0_IMR, mask);
    } else {
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) & ~(1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}

void irq_disable(int irq_num) {
    if (irq_num < IRQ_PIC_START) {
        return;
    }

    irq_num -= IRQ_PIC_START;
    if (irq_num < 8) {
        uint8_t mask = inb(PIC0_IMR) | (1 << irq_num);
        outb(PIC0_IMR, mask);
    } else {
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) | (1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}

/*irq_disable_global() 函数的目的是全局禁用中断。
在x86架构中，cli（Clear Interrupts）指令用于清除中断标志位，从而禁止CPU响应外部硬件中断。这个指令通常在操作系统的内核代码中使用，
以确保在执行某些关键操作时不会被中断。
当cli()被执行后：
CPU将不会响应任何外部硬件中断请求，包括定时器中断。
这并不意味着定时器停止工作，定时器可能仍然在计数，但是CPU不会接收到相关的中断信号。
软件中断（如通过int指令触发的中断）也不会被阻止。
这种全局中断禁用通常用于以下情况：
系统初始化或关键代码段，需要保证代码的原子性。
避免中断处理中的递归调用或死锁。
在某些特定的硬件操作中，可能需要确保在操作完成之前不被中断。*/
void irq_disable_global (void) {
    cli();
}

void irq_enable_global (void) {
    sti();
}

void pic_send_eoi(int irq_num) {
    irq_num -= IRQ_PIC_START;

    // 从片也可能需要发送EOI
    if (irq_num >= 8) {
        outb(PIC1_OCW2, PIC_OCW2_EOI);
    }

    outb(PIC0_OCW2, PIC_OCW2_EOI);
}

irq_state_t irq_enter_protection(void) {
    irq_state_t state = read_eflags();
    irq_disable_global();
    return state;
}
void irq_leave_protection(irq_state_t state) {
    write_eflags(state);
    irq_enable_global();
}