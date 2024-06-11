#include "init.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/timer.h"
#include "tools/log.h"
#include "os_cfg.h"

void kernel_init(boot_info_t* boot_info) {
    cpu_init();
    log_init();
    irq_init();
    timer_init();
}

void init_main(void) {
    log_printf("Kernerl is runniing");
    log_printf("Version: %s %s", OS_VERSION, "x86 os");

    log_printf("%d %d %x %c", -123, 123456, 0x12345, 'a');
    // int a = 3/0;
    // irq_enable_global(); // 设置了8259之后还要这样开启全局中断
    for (;;) {}
}