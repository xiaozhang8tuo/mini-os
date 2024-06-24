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

void kernel_init(boot_info_t* boot_info) {
    // ASSERT(boot_info->ram_region_count != 0);
    // ASSERT(4<2);
    
    cpu_init();
    log_init();
    irq_init();
    timer_init();
}

static task_t init_task;
static uint32_t init_task_stack[1024];// init_task_entry 的栈
static task_t first_task;

void init_task_entry() {
    int count = 0;
    for (;;) {
        log_printf("init task: %d", count++);
        task_switch_from_to(&init_task, &first_task);
    }
}

void list_test(void) {
    list_t list;
    list_init(&list);
    log_printf("list: first=0x%x, last=0x%x,count=%d",
    list_first(&list), list_last(&list), list_count(&list));
}

void init_main(void) {
    list_test();

    log_printf("Kernerl is runniing");
    log_printf("Version: %s %s", OS_VERSION, "x86 os");

    // log_printf("%d %d %x %c", -123, 123456, 0x12345, 'a');
    // int a = 3/0;
    // irq_enable_global(); // 设置了8259之后还要这样开启全局中断

    task_init(&init_task, (uint32_t)init_task_entry, (uint32_t)&init_task_stack[1024]);//这里取底部是因为，恢复现场时，栈需要pop的时候是从底往上的
    task_init(&first_task, 0, 0);
    write_tr(first_task.tss_sel);

    int count = 0;
    for (;;) {
        log_printf("int main: %d", count++);
        task_switch_from_to(&first_task, &init_task);
    }

}