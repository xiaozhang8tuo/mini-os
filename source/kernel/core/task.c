#include "core/task.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "tools/log.h"
static task_manager_t task_manager;
static uint32_t idle_task_stack[IDLE_TASK_STACK_SIZE];
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
    kernel_memcpy(task->name, (void*)name, TASK_NAME_SIZE);
    task->state = TASK_CREATED;
    task->time_ticks = TASK_TIME_SLICE_DEFAULT;
    task->sleep_ticks = 0;
    task->slice_ticks = task->time_ticks;
    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);
    irq_state_t state = irq_enter_protection();
    task_set_ready(task);
    list_insert_last(&task_manager.task_list, &task->all_node);
    irq_leave_protection(state);
    return 0;
}

void simple_switch(uint32_t** from, uint32_t* to);

void task_switch_from_to(task_t* from, task_t* to) {
    switch_to_tss(to->tss_sel);
    // simple_switch(&from->stack, to->stack);
}

static void idle_task_entry (void) {
    for (;;) {
        hlt();
    }
}

void task_manager_init(void) {
    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);
    task_manager.curr_task = (task_t *)0;

    task_init(&task_manager.idle_task, "idle_task", (uint32_t)idle_task_entry, (uint32_t)(idle_task_stack+IDLE_TASK_STACK_SIZE));
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
    if (task == &task_manager.idle_task) {
        return;
    }

    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}

void task_set_block(task_t* task) {
    if (task == &task_manager.idle_task) {
        return;
    }

    list_remove(&task_manager.ready_list, &task->run_node);
}

task_t * taks_next_run(void) {
    if (list_count(&task_manager.ready_list) == 0) {
        return &task_manager.idle_task;
    }

    list_node_t* task_node = list_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

task_t * task_current(void) {
    return task_manager.curr_task;
}

void task_dispatch(void) {
    // irq_state_t state = irq_enter_protection();
    task_t* to = taks_next_run();
    if (to != task_manager.curr_task) {
        task_t* from = task_current();
        task_manager.curr_task = to;
        to->state = TASK_RUNNING;

        task_switch_from_to(from, to);
    }
    // irq_leave_protection(state);
}

int sys_sched_yield(void) {
    irq_state_t state = irq_enter_protection();
    if (list_count(&task_manager.ready_list) > 1) {
        task_t* curr_task = task_current();

        task_set_block(curr_task);
        task_set_ready(curr_task);

        task_dispatch();
    }
    irq_leave_protection(state);
    return 0;
}

void task_timer_tick(void) {
    task_t* curr_task = task_current();
    if (--curr_task->slice_ticks == 0) {
        curr_task->slice_ticks = curr_task->time_ticks;
        
        task_set_block(curr_task);
        task_set_ready(curr_task);

        task_dispatch();
    }

    list_node_t* curr = list_first(&task_manager.sleep_list);
    while (curr)
    {
        list_node_t* next = list_node_next(curr);
        task_t* task = list_node_parent(curr, task_t, run_node);
        if (--task->sleep_ticks == 0) {
            task_set_wakeup(task);
            task_set_ready(task);
        }
        curr = next;
    }
    task_dispatch();
}

void task_set_sleep(task_t* task, uint32_t ticks) {
    if (ticks == 0) {
        return;
    }
    task->sleep_ticks = ticks;
    task->state = TASK_SLEEP;
    list_insert_last(&task_manager.sleep_list, &task->run_node);
}

void task_set_wakeup(task_t* task) {
    list_remove(&task_manager.sleep_list, &task->run_node);
}

void sys_sleep(uint32_t ms) {
    irq_state_t state = irq_enter_protection();
    task_set_block(task_manager.curr_task);
    task_set_sleep(task_manager.curr_task, (ms+OS_TICKS_MS-1)/OS_TICKS_MS);// 向上取整
    task_dispatch();
    irq_leave_protection(state);
}