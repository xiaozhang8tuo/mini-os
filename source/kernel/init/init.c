#include "init.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/timer.h"
#include "tools/log.h"
#include "os_cfg.h"
#include "tools/klib.h"
#include "core/task.h"
#include "tools/list.h"
#include "ipc/sem.h"
#include "core/memory.h"
#include "dev/console.h"
#include "dev/kbd.h"
#include "fs/fs.h"

void kernel_init(boot_info_t* boot_info) {
    // ASSERT(boot_info->ram_region_count != 0);
    // ASSERT(4<2);
    cpu_init();
    irq_init();
    memory_init(boot_info);
    log_init();
    fs_init();
    timer_init();

    task_manager_init();
    kbd_init();
}

// static task_t init_task;
// static uint32_t init_task_stack[1024];// init_task_entry 的栈
// static sem_t sem;

// void init_task_entry() {
//     int count = 0;
//     for (;;) {
//         // sem_wait(&sem);
//         log_printf("init task: %d", count++);
//         // sys_sleep(1000);
//         // sys_yield();
//     }
// }

void list_test(void) {
    list_t list;
    list_node_t nodes[5];
    list_init(&list);
    log_printf("list: first=0x%x, last=0x%x,count=%d",
    list_first(&list), list_last(&list), list_count(&list));

    // 插入
    for (int i = 0; i < 5; i++) {
        list_node_t * node = nodes + i;
        log_printf("insert first to list: %d, 0x%x", i, (uint32_t)node);
        list_insert_first(&list, node);
    }
    log_printf("list: first=0x%x, last=0x%x, count=%d", list_first(&list), list_last(&list), list_count(&list));

    for (int i = 0; i < 5; i++) {
        list_node_t * node = list_remove_first(&list);
        log_printf("remove first from list: %d, 0x%x", i, (uint32_t)node);
    }
    log_printf("list: first=0x%x, last=0x%x, count=%d", list_first(&list), list_last(&list), list_count(&list));


    list_init(&list);
    for (int i = 0; i < 5; i++) {
        list_node_t * node = nodes + i;
        log_printf("insert last to list: %d, 0x%x", i, (uint32_t)node);
        list_insert_last(&list, node);
    }

    log_printf("list: first=0x%x, last=0x%x, count=%d", list_first(&list), list_last(&list), list_count(&list));
        for (int i = 0; i < 5; i++) {
        list_node_t * node = nodes + i;
        log_printf("remove first from list: %d, 0x%x", i, (uint32_t)node);
        list_remove(&list, node);
    }
    log_printf("list: first=0x%x, last=0x%x, count=%d", list_first(&list), list_last(&list), list_count(&list));



    struct type_t
    {
        int i;
        list_node_t node;
        /* data */
    } v = {0x123456};

    struct type_t* a = (struct type_t *)0;
    uint32_t addr = (uint32_t)&a->node;
    uint32_t addr_p = offset_in_parent(struct type_t, node);

    list_node_t* v_node = &v.node;
    struct type_t* p = list_node_parent(v_node, struct type_t, node);
    if (p->i != 0x123456) {
        log_printf("error");
    }
    
}

void move_to_first_task() {
    task_t * curr = task_current();
    ASSERT(curr != 0);

    tss_t * tss = &(curr->tss);
    __asm__ __volatile__ (
        "push %[ss]\n\t"
        "push %[esp]\n\t"
        "push %[eflags]\n\t"
        "push %[cs]\n\t"
        "push %[eip]\n\t"
        "iret\n\t"::[ss]"r"(tss->ss), [esp]"r"(tss->esp), [eflags]"r"(tss->eflags),
        [cs]"r"(tss->cs), [eip]"r"(tss->eip)
    );
}


void init_main(void) {
    // list_test();

    log_printf("Kernerl is runniing");
    log_printf("Version: %s %s", OS_VERSION, "x86 os");

    // log_printf("%d %d %x %c", -123, 123456, 0x12345, 'a');
    // int a = 3/0;

    // task_init(&init_task, "init task", (uint32_t)init_task_entry, (uint32_t)&init_task_stack[1024]);//这里取底部是因为，恢复现场时，栈需要pop的时候是从底往上的
    // task_first_init();
    // sem_init(&sem, 0);
    // irq_enable_global(); // 设置了8259之后还要这样开启全局中断

    // int count = 0;
    // for (;;) {
    //     log_printf("main task: %d", count++);
    //     // sem_notify(&sem);
    //     // sys_sleep(1000);
    //     // sys_yield();
    // }

    task_first_init();
    move_to_first_task();

}