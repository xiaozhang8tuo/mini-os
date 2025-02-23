#include "lib_syscall.h"
#include <stdio.h>

char cmd_buf[256];
int main (int argc, char **argv) {
#if 0
    sbrk(0);
    sbrk(100);
    sbrk(200);
    sbrk(4096*2 + 200);// sbrk(0): end = 0x81005330 sbrk(8392): end = 0x81007524, 0x81007524-0x81005330=100 + 200 + (4096*2 + 200)
    sbrk(4096*5 + 1234);

    printf("abef\b\b\b\bcd\n");    // \b:backword 输出cdef
    printf("abcd\x7f;fg\n");   // 7f:删除 输出 abc;fg
    printf("\0337Hello,word!\0338123\n");  // ESC 7,8 输出123lo,word!
    printf("\033[31;42mHello,word!\033[39;49m123\n");  // ESC [pn m, Hello,world红色，其余绿色
    printf("123\033[2DHello,word!\n");  // 光标左移2，1Hello,word!
    printf("123\033[2CHello,word!\n");  // 光标右移2，123  Hello,word!

    printf("\033[31m");  // ESC [pn m, Hello,world红色，其余绿色
    printf("\033[10;10H test!\n");  // 定位到10, 10，test!
    printf("\033[20;20H test!\n");  // 定位到20, 20，test!
    printf("\033[32;25;39m123\n");  // ESC [pn m, Hello,world红色，其余绿色  

    printf("\033[2J\n");   // clear screen
#endif
	open(argv[0], 0);
    dup(0);     // 标准输出
    dup(0);     // 标准错误输出

    printf("hello from shell\n");
    puts("TEST!");
    for (int i = 0; i < argc; i++) {
//        print_msg("arg: %s", (int)argv[i]);
        printf("arg: %s\n", (int)argv[i]);
    }

    // // 创建一个自己的副本
    // fork();

    // yield();

    fprintf(stderr, "stderr output\n");
    puts("sh >>");

    for (;;) {
        gets(cmd_buf);
        puts(cmd_buf);
        // print_msg("pid=%d", getpid());
        printf("pid=%d\n", getpid());
        msleep(1000);
    }
}