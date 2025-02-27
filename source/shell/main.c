#include "lib_syscall.h"
#include <stdio.h>
#include <string.h>
#include "main.h"

static cli_t cli;
static const char * promot = "sh >>";       // 命令行提示符

/**
 * 显示命令行提示符
 */
static void show_promot(void) {
    printf("%s", cli.promot);
    fflush(stdout);
}

/**
 * help命令
 */
static int do_help(int argc, char **argv) {
    return 0;
}

// 命令列表
static const cli_cmd_t cmd_list[] = {
    {
        .name = "help",
		.useage = "help -- list support command",
		.do_func = do_help,
    },
};

/**
 * 命令行初始化
 */
static void cli_init(const char * promot, const cli_cmd_t * cmd_list, int cnt) {
    cli.promot = promot;
    
    memset(cli.curr_input, 0, CLI_INPUT_SIZE);
    
    cli.cmd_start = cmd_list;
    cli.cmd_end = cmd_list + cnt;
}

int main (int argc, char **argv) {
	open(argv[0], 0);
    dup(0);     // 标准输出
    dup(0);     // 标准错误输出

   	cli_init(promot, cmd_list, sizeof(cmd_list) / sizeof(cli_cmd_t));
    for (;;) {
        show_promot();
        gets(cli.curr_input);
    }

    return 0;
}


// char cmd_buf[256];
// int main (int argc, char **argv) {
// #if 0
//     sbrk(0);
//     sbrk(100);
//     sbrk(200);
//     sbrk(4096*2 + 200);// sbrk(0): end = 0x81005330 sbrk(8392): end = 0x81007524, 0x81007524-0x81005330=100 + 200 + (4096*2 + 200)
//     sbrk(4096*5 + 1234);

//     printf("abef\b\b\b\bcd\n");    // \b:backword 输出cdef
//     printf("abcd\x7f;fg\n");   // 7f:删除 输出 abc;fg
//     printf("\0337Hello,word!\0338123\n");  // ESC 7,8 输出123lo,word!
//     printf("\033[31;42mHello,word!\033[39;49m123\n");  // ESC [pn m, Hello,world红色，其余绿色
//     printf("123\033[2DHello,word!\n");  // 光标左移2，1Hello,word!
//     printf("123\033[2CHello,word!\n");  // 光标右移2，123  Hello,word!

//     printf("\033[31m");  // ESC [pn m, Hello,world红色，其余绿色
//     printf("\033[10;10H test!\n");  // 定位到10, 10，test!
//     printf("\033[20;20H test!\n");  // 定位到20, 20，test!
//     printf("\033[32;25;39m123\n");  // ESC [pn m, Hello,world红色，其余绿色  

//     printf("\033[2J\n");   // clear screen
// #endif
// 	open(argv[0], 0);
//     dup(0);     // 标准输出
//     dup(0);     // 标准错误输出

//     printf("hello from shell\n");
//     puts("TEST!");
//     for (int i = 0; i < argc; i++) {
// //        print_msg("arg: %s", (int)argv[i]);
//         printf("arg: %s\n", (int)argv[i]);
//     }

//     // // 创建一个自己的副本
//     // fork();

//     // yield();

//     fprintf(stderr, "stderr output\n");
//     puts("sh >>");

//     for (;;) {
//         gets(cmd_buf);
//         puts(cmd_buf);
//         // print_msg("pid=%d", getpid());
//         printf("pid=%d\n", getpid());
//         msleep(1000);
//     }
// }