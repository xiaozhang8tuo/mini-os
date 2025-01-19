#include "lib_syscall.h"
#include <stdio.h>

int main (int argc, char **argv) {
    sbrk(0);
    sbrk(100);
    sbrk(200);
    sbrk(4096*2 + 200);// sbrk(0): end = 0x81005330 sbrk(8392): end = 0x81007524, 0x81007524-0x81005330=100 + 200 + (4096*2 + 200)
    sbrk(4096*5 + 1234);

    printf("abef\b\b\b\bcd\n");    // \b:backword 输出cdef
    printf("abcd\x7f;fg\n");   // 7f:删除 输出 abc;fg
    printf("\0337Hello,word!\0338123\n");  // ESC 7,8 输出123lo,word!
    printf("\033[31;42mHello,word!\033[39;49m123\n");  // ESC [pn m, Hello,world红色，其余绿色

    printf("hello from shell\n");
    puts("TEST!");
    for (int i = 0; i < argc; i++) {
//        print_msg("arg: %s", (int)argv[i]);
        printf("arg: %s\n", (int)argv[i]);
    }

    // 创建一个自己的副本
    fork();

    yield();

    for (;;) {
        // print_msg("pid=%d", getpid());
        printf("pid=%d\n", getpid());
        msleep(1000);
    }
}