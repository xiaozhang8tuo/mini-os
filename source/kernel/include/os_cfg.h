#ifndef OS_CFG_H
#define OS_CFG_H

#define GDT_TABLE_SIZE 256

#define KERNEL_SELECTOR_CS (1 * 8)
#define KERNEL_SELECTOR_DS (2 * 8)
#define SELECTOR_SYSCALL   (3 * 8)


#define KERNEL_STACK_SIZE (8 * 1024)
#define IDLE_TASK_STACK_SIZE 1024

#define OS_TICKS_MS 10

#define OS_VERSION "1.0.0"

#define TASK_NR             128            // 进程的数量

#define ROOT_DEV            DEV_DISK, 0xb1  // 根目录所在的设备


#endif