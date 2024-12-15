#include "lib_syscall.h"
#include <stdio.h>

int main (int argc, char **argv) {
    sbrk(0);
    sbrk(100);
    sbrk(200);
    sbrk(4096*2 + 200);// sbrk(0): end = 0x81005330 sbrk(8392): end = 0x81007524, 0x81007524-0x81005330=100 + 200 + (4096*2 + 200)
    sbrk(4096*5 + 1234);

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