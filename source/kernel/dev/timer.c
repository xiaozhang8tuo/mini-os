#include "comm/types.h"
#include "dev/timer.h"
#include "os_cfg.h"
#include "cpu/irq.h"
#include "comm/cpu_instr.h"
#include "core/task.h"

static uint32_t sys_tick;

void exception_handler_timer (void);

void do_handler_timer(exception_frame_t* frame) {
    sys_tick++;

    // 先发EOI，而不是放在最后
    // 放最后将从任务中切换出去之后，除非任务再切换回来才能继续噢应
    // 这样才能让中断按照时间重复触发，否则只有一次
    pic_send_eoi(IRQ0_TIMER);
    task_timer_tick();
}

void timer_init (void) {
    uint32_t reload_count  = PIT_OSC_FREQ * OS_TICKS_MS / 1000;
    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNLE0 | PIT_LOAD_LOHI | PIT_MODE3);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF);
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count>>8) & 0XFF);

    irq_install(IRQ0_TIMER, (irq_handler_t)exception_handler_timer);
    irq_enable(IRQ0_TIMER);
}