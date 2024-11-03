#include "core/task.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "tools/log.h"
#include "cpu/mmu.h"
#include "core/memory.h"
#include "core/syscall.h"

static task_manager_t task_manager;
static uint32_t idle_task_stack[IDLE_TASK_STACK_SIZE];
static task_t task_table[TASK_NR];      // 用户进程表
static mutex_t task_table_mutex;        // 进程表互斥访问锁


// 初始化tss结构,设置入口地址,分配选择子等等
static int tss_init(task_t* task, int flag, uint32_t entry, uint32_t esp) {
    int tss_sel = gdt_alloc_desc();
    if (tss_sel < 0) {
        log_printf("alloc tss failed.\r\n");
        return -1;
    }

    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t),
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS
    );

    kernel_memset(&(task->tss), 0, sizeof(tss_t));

    // 分配内核栈，得到的是物理地址
    uint32_t kernel_stack = memory_alloc_page();
    if (kernel_stack == 0) {
        goto tss_init_failed;
    }

    int code_sel, data_sel;
    if (flag & TASK_FLAG_SYSTEM) {
        code_sel = KERNEL_SELECTOR_CS;
        data_sel = KERNEL_SELECTOR_DS;
    } else {
        // 注意加了RP3,不然将产生段保护错误
        code_sel = task_manager.app_code_sel | SEG_CPL3;
        data_sel = task_manager.app_data_sel | SEG_CPL3;
    }

    task->tss.eip = entry;
    task->tss.esp = esp ? esp : kernel_stack + MEM_PAGE_SIZE;  // 未指定栈则用内核栈，即运行在特权级0的进程
    task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE;
    task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.eflags = EFLAGS_DEFAULT| EFLAGS_IF;
    task->tss.es = task->tss.ss = task->tss.ds = task->tss.fs 
            = task->tss.gs = data_sel;   // 全部采用同一数据段
    task->tss.cs = code_sel; 
    task->tss.iomap = 0;

    // 页表初始化
    uint32_t page_dir = memory_create_uvm();
    if (page_dir == 0) {
        goto tss_init_failed;
    }
    task->tss.cr3 = page_dir;

    task->tss_sel = tss_sel;

    return 0;
tss_init_failed:
    gdt_free_sel(tss_sel);

    if (kernel_stack) {
        memory_free_page(kernel_stack);
    }
    return -1;
}

int task_init(task_t* task, const char* name, int flag, uint32_t entry, uint32_t esp) {
    ASSERT(task != (task_t *)0);

    tss_init(task, flag, entry, esp);
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
    task->parent = (task_t *)0;
    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);
    irq_state_t state = irq_enter_protection();
    task->pid = (uint32_t)task;   // 使用地址，能唯一
    task_set_ready(task);
    list_insert_last(&task_manager.task_list, &task->all_node);
    irq_leave_protection(state);
    return 0;
}


/**
 * @brief 任务任务初始时分配的各项资源
 */
void task_uninit (task_t * task) {
    // 选择子
    if (task->tss_sel) {
        gdt_free_sel(task->tss_sel);
    }

    // 栈空间
    if (task->tss.esp0) {
        memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE);
    }

    // 页表
    if (task->tss.cr3) {
        // 没有分配空间，暂时不写
        //memory_destroy_uvm(task->tss.cr3);
    }

    kernel_memset(task, 0, sizeof(task_t));
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
    kernel_memset(task_table, 0, sizeof(task_table));
    mutex_init(&task_table_mutex);

    int sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);
    task_manager.app_data_sel = sel;

    sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);
    task_manager.app_code_sel = sel;

    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);
    task_manager.curr_task = (task_t *)0;

    task_init(&task_manager.idle_task, "idle_task", TASK_FLAG_SYSTEM, 
                (uint32_t)idle_task_entry, 0);     // 运行于内核模式，无需指定特权级3的栈
}

void task_first_init(void) {
    void first_task_entry(void);
    extern uint8_t s_first_task[], e_first_task[];

    // 分配的空间比实际存储的空间要大一些，多余的用于放置栈
    uint32_t copy_size = (uint32_t)(e_first_task - s_first_task);
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE;
    ASSERT(copy_size < alloc_size);

    uint32_t first_start = (uint32_t)first_task_entry;

    task_init(&task_manager.first_task, "first task", 0, first_task_entry, first_start + alloc_size);
    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task = &task_manager.first_task;

    mmu_set_page_dir(task_manager.first_task.tss.cr3);

    // 分配一页内存供代码存放使用，然后将代码复制过去, PTE_U :用户态特权级3可以读
    memory_alloc_page_for(first_start,  alloc_size, PTE_P | PTE_W | PTE_U);
    kernel_memcpy((void *)first_start, (void *)s_first_task, copy_size);
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

/**
 * 返回任务的pid
 */
int sys_getpid (void) {
    task_t * curr_task = task_current();
    return curr_task->pid;
}

/**
 * 分配一个任务结构
 */
static task_t * alloc_task (void) {
    task_t * task = (task_t *)0;

    mutex_lock(&task_table_mutex);
    for (int i = 0; i < TASK_NR; i++) {
        task_t * curr = task_table + i;
        if (curr->name[0] == 0) {
            task = curr;
            break;
        }
    }
    mutex_unlock(&task_table_mutex);

    return task;
}

/**
 * 释放任务结构
 */
static void free_task (task_t * task) {
    mutex_lock(&task_table_mutex);
    task->name[0] = 0;
    mutex_unlock(&task_table_mutex);
}

/**
 * 创建进程的副本
 */
int sys_fork (void) {
    task_t * parent_task = task_current();

    // 分配任务结构
    task_t * child_task = alloc_task();
    if (child_task == (task_t *)0) {
        goto fork_failed;
    }
    syscall_frame_t * frame = (syscall_frame_t *)(parent_task->tss.esp0 - sizeof(syscall_frame_t));

    // 对子进程进行初始化, 并对必要的字段进行调整
    // 其中esp要减去系统调用的总参数字节大小，因为其是通过正常的ret返回, 而没有走系统调用处理的ret(参数个数返回)
    // 这里要注意子进程的esp不能直接设置成父进程的esp,要把传入的args这块pop出来. 子进程执行的位置,是父进程执行完毕lcall之后的位置
    int err = task_init(child_task,  parent_task->name, 0, frame->eip,
                        frame->esp + sizeof(uint32_t)*SYSCALL_PARAM_COUNT);
    if (err < 0) {
        goto fork_failed;
    }

    // 从父进程的栈中取部分状态，然后写入tss。
    // 注意检查esp, eip等是否在用户空间范围内，不然会造成page_fault
    tss_t * tss = &child_task->tss;
    tss->eax = 0;                       // 子进程返回0, 这就是为什么getpid返回值可以区分父子进程的原因
    tss->ebx = frame->ebx;
    tss->ecx = frame->ecx;
    tss->edx = frame->edx;
    tss->esi = frame->esi;
    tss->edi = frame->edi;
    tss->ebp = frame->ebp;

    tss->cs = frame->cs;
    tss->ds = frame->ds;
    tss->es = frame->es;
    tss->fs = frame->fs;
    tss->gs = frame->gs;
    tss->eflags = frame->eflags;

    child_task->parent = parent_task;
    // 复制父进程的内存空间到子进程，暂时使用相同的页表
    child_task->tss.cr3 = parent_task->tss.cr3;

    // 创建成功，返回子进程的pid
    return child_task->pid;
fork_failed:
    if (child_task) {
        task_uninit (child_task);
        free_task(child_task);
    }
    return -1;
}