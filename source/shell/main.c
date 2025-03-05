#include "lib_syscall.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include <getopt.h>
#include <stdlib.h>
#include <sys/file.h>

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
    const cli_cmd_t * start = cli.cmd_start;

    while (start < cli.cmd_end) {
        printf("%s %s\n",  start->name, start->useage);
        start++;
    }
    return 0;
}

/**
 * 清屏命令
 */
static int do_clear (int argc, char ** argv) {
    printf("%s", ESC_CLEAR_SCREEN);
    printf("%s", ESC_MOVE_CURSOR(0, 0));
    return 0;
}

/**
 * 回显命令
 */
static int do_echo (int argc, char ** argv) {
    // 只有一个参数，需要先手动输入，再输出
    if (argc == 1) {
        char msg_buf[128];

        fgets(msg_buf, sizeof(msg_buf), stdin);
        msg_buf[sizeof(msg_buf) - 1] = '\0';
        puts(msg_buf);
        return 0;
    }

    // https://www.cnblogs.com/yinghao-liu/p/7123622.html
    // optind是下一个要处理的元素在argv中的索引
    // 当没有选项时，变为argv第一个不是选项元素的索引。
    int count = 1;    // 缺省只打印一次
    int ch;
    while ((ch = getopt(argc, argv, "n:h")) != -1) {
        switch (ch) {
            case 'h':
                puts("echo echo any message");
                puts("Usage: echo [-n count] msg");
                optind = 1;        // getopt需要多次调用，需要重置
                return 0;
            case 'n':
                count = atoi(optarg);
                break;
            case '?':
                if (optarg) {
                    fprintf(stderr, "Unknown option: -%s\n", optarg);
                }
                optind = 1;        // getopt需要多次调用，需要重置
                return -1;
        }
    }

    // 索引已经超过了最后一个参数的位置，意味着没有传入要发送的信息
    if (optind > argc - 1) {
        fprintf(stderr, "Message is empty \n");
        optind = 1;        // getopt需要多次调用，需要重置
        return -1;
    }

    // 循环打印消息
    char * msg = argv[optind];
    for (int i = 0; i < count; i++) {
        puts(msg);
    }
    optind = 1;        // getopt需要多次调用，需要重置
    return 0;
}

/**
 * 程序退出命令
 */
static int do_exit (int argc, char ** argv) {
    exit(0);
    return 0;
}



// 命令列表
static const cli_cmd_t cmd_list[] = {
    {
        .name = "help",
		.useage = "help -- list support command",
		.do_func = do_help,
    },
    {
        .name = "clear",
		.useage = "clear -- clear the screen",
		.do_func = do_clear,
    },
	{
		.name = "echo",
		.useage = "echo [-n count] msg  -- echo something",
		.do_func = do_echo,
	},
    {
        .name = "quit",
        .useage = "quit from shell",
        .do_func = do_exit,
    }
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

/**
 * 在内部命令中搜索
 */
static const cli_cmd_t * find_builtin (const char * name) {
    for (const cli_cmd_t * cmd = cli.cmd_start; cmd < cli.cmd_end; cmd++) {
        if (strcmp(cmd->name, name) != 0) {
            continue;
        }

        return cmd;
    }

    return (const cli_cmd_t *)0;
}

/**
 * 运行内部命令
 */
static void run_builtin (const cli_cmd_t * cmd, int argc, char ** argv) {
    int ret = cmd->do_func(argc, argv);
    if (ret < 0) {
        fprintf(stderr,ESC_COLOR_ERROR"error: %d\n"ESC_COLOR_DEFAULT, ret);
    }
}

/**
 * 试图运行当前文件
 */
static void run_exec_file (const char * path, int argc, char ** argv) {
    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed: %s", path);
    } else if (pid == 0) {
        // 以下供测试exit使用
        for (int i = 0; i < argc; i++) {
            msleep(1000);
            printf("arg %d = %s\n", i, argv[i]);
        }
        exit(-1);

        // 子进程
        // int err = execve(path, argv, (char * const *)0);
        // if (err < 0) {
        //     fprintf(stderr, "exec failed: %s", path);
        // }
        // exit(-1);
    } else {
		// 等待子进程执行完毕
        int status;
        int pid = wait(&status);
        fprintf(stderr, "cmd %s result: %d, pid = %d\n", path, status, pid);
    }
}


int main (int argc, char **argv) {
	open(argv[0], O_RDWR);
    dup(0);     // 标准输出
    dup(0);     // 标准错误输出

   	cli_init(promot, cmd_list, sizeof(cmd_list) / sizeof(cli_cmd_t));
	for (;;) {
        show_promot();

        // 获取输入的字符串，然后进行处理.
        // 注意，读取到的字符串结尾中会包含换行符和0
        char * str = fgets(cli.curr_input, CLI_INPUT_SIZE, stdin);
        if (str == (char *)0) {
            break;
        }

        // 读取的字符串中结尾可能有换行符
        char * cr = strchr(cli.curr_input, '\n');
        if (cr) {
            *cr = '\0';
        }
        cr = strchr(cli.curr_input, '\r');
        if (cr) {
            *cr = '\0';
        }

        int argc = 0;
        char * argv[CLI_MAX_ARG_COUNT];
        memset(argv, 0, sizeof(argv));

        const char * space = " ";
        char *token = strtok(cli.curr_input, space);
        while (token) {
            argv[argc++] = token;
            token = strtok(NULL, space);
        }

        if (argc == 0) {
            continue;
        }

        const cli_cmd_t * cmd = find_builtin(argv[0]);
        if (cmd) {
            run_builtin(cmd, argc, argv);
            continue;
        }

        // 测试程序，运行虚拟的程序
        run_exec_file("", argc, argv);

        // 试图作为外部命令执行。只检查文件是否存在，不考虑是否可执行
        // const char * path = find_exec_path(argv[0]);
        // if (path) {
        //     run_exec_file(path, argc, argv);
        //     continue;
        // }

        // 找不到命令，提示错误
        fprintf(stderr, ESC_COLOR_ERROR"Unknown command: %s\n"ESC_COLOR_DEFAULT, cli.curr_input);
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