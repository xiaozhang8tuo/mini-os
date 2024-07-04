#include "core/task.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"

static task_manager_t task_manager;

// 初始化tss结构,设置入口地址,分配选择子等等
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

int task_init(task_t* task, const char* name, uint32_t entry, uint32_t esp) {
    ASSERT(task != (task_t *)0);

    tss_init(task, entry, esp);
    // uint32_t* pesp = (uint32_t *)esp;
    // if (pesp) {
    //     *(--pesp) = entry; //这里填写任务的返回地址，恢复现场的时候ret用
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     task->stack = pesp;
    // }
    kernel_memcpy(task->name, name, TASK_NAME_SIZE);
    task->state = TASK_CREATED;
    list_node_init(&task->all_node);
    list_node_init(&task->run_node);

    task_set_ready(task);
    list_insert_last(&task_manager.task_list, &task->all_node);
    return 0;
}

void simple_switch(uint32_t** from, uint32_t* to);

void task_switch_from_to(task_t* from, task_t* to) {
    switch_to_tss(to->tss_sel);
    // simple_switch(&from->stack, to->stack);
}

void task_manager_init(void) {
    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
}

void task_first_init(void) {
    task_init(&task_manager.first_task, "first task", 0, 0);
    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task = &task_manager.first_task;
}

task_t* task_first_task(void) {
    return &task_manager.first_task;
}


void task_set_ready(task_t* task) {
    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}

void task_set_block(task_t* task) {
    list_remove(&task_manager.ready_list, &task->run_node);
}