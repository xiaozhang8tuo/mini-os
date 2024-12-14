#include "lib_syscall.h"
#include <stdio.h>

int main (int argc, char **argv) {

    printf("hello from shell");
    for (int i = 0; i < argc; i++) {
        print_msg("arg: %s", (int)argv[i]);
    }

    // 创建一个自己的副本
    fork();

    yield();

    for (;;) {
        print_msg("pid=%d", getpid());
        msleep(1000);
    }
}