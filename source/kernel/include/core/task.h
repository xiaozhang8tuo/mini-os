#ifndef TASK_H
#define TASK_H
#include "comm/types.h"
#include "cpu/cpu.h"

typedef struct _task_t {
    // uint32_t* stack;

    tss_t tss;
    int tss_sel;
}task_t;

int task_init(task_t* task, uint32_t entry, uint32_t esp);
// 从from任务切换至to任务
void task_switch_from_to(task_t* from, task_t* to);

#endif