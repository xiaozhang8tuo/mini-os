#include "core/task.h"
#include "tools/klib.h"
#include "os_cfg.h"

static int tss_init(task_t* task, uint32_t entry, uint32_t esp) {
    int tss_sel = gdt_alloc_desc();
    if (tss_sel < 0) {
        log_printf("alloc tss failed.\r\n");
        return -1;
    }

    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t),
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS
    );

    kernel_memset(&(task->tss), 0, sizeof(tss_t));
    task->tss.eip = entry;//入口地址
    task->tss.esp = task->tss.esp0 = esp;// 程序运行在特权级0
    task->tss.ss = task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = KERNEL_SELECTOR_DS;
    task->tss.cs = KERNEL_SELECTOR_CS;
    task->tss.eflags = EFLAGS_DEFAULT | EFLAGS_IF;

    task->tss_sel = tss_sel;

    return 0;
}

int task_init(task_t* task, uint32_t entry, uint32_t esp) {
    ASSERT(task != (task_t *)0);

    // tss_init(task, entry, esp);
    uint32_t* pesp = (uint32_t *)esp;
    if (pesp) {
        *(--pesp) = entry; //这里填写任务的返回地址，恢复现场的时候ret用
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        task->stack = pesp;
    }

    return 0;
}

void simple_switch(uint32_t** from, uint32_t* to);

void task_switch_from_to(task_t* from, task_t* to) {
    // switch_to_tss(to->tss_sel);
    simple_switch(&from->stack, to->stack);
}