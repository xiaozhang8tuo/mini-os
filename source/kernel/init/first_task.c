#include "core/task.h"
#include "tools/log.h"
#include "applib/lib_syscall.h"

int first_task_main (void) {
    int count = 3;

    int pid = getpid();
    print_msg("first task id=%d", pid);
    
    pid = fork();
    if (pid < 0) {
        print_msg("create child proc failed.", 0);
    } else if (pid == 0) {
        print_msg("child: %d", count);
    } else {
        print_msg("child task id=%d", pid);
        print_msg("parent: %d", count);
    }

    pid = getpid();
    for (;;) {
        print_msg("task id=%d", pid);
        msleep(1000);
    }

    return 0;
}