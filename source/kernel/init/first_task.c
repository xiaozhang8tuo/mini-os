#include "core/task.h"
#include "tools/log.h"
#include "applib/lib_syscall.h"

int first_task_main (void) {
    // 可将task_manager添加到观察窗口中，找到curr_task.pid比较
    int pid = getpid();

    for (;;) {
        msleep(1000);
    }

    return 0;
}