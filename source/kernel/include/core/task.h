#ifndef TASK_H
#define TASK_H
#include "comm/types.h"
#include "cpu/cpu.h"
#include "tools/list.h"
#include "fs/file.h"

#define TASK_NAME_SIZE 32
#define TASK_TIME_SLICE_DEFAULT 10
#define TASK_OFILE_NR				128			// 最多支持打开的文件数量

#define TASK_FLAG_SYSTEM       	(1 << 0)		// 系统任务

typedef struct _task_args_t {
	uint32_t ret_addr;		// 返回地址，无用
	uint32_t argc;
	char **argv;
}task_args_t;

typedef struct _task_t {
    // uint32_t* stack;
    enum {
        TASK_CREATED,
        TASK_RUNNING,
        TASK_SLEEP,
        TASK_READY,
        TASK_WAITING,
        TASK_ZOMBIE,
    }state;

    int sleep_ticks;
    int time_ticks;
    int slice_ticks;
    int status;				// 进程执行结果

    file_t * file_table[TASK_OFILE_NR];	// 任务最多打开的文件数量

    char name[TASK_NAME_SIZE];
    int pid;				// 进程的pid
    struct _task_t * parent;		// 父进程
	uint32_t heap_start;		// 堆的顶层地址
	uint32_t heap_end;			// 堆结束地址
	
    list_node_t run_node;
    list_node_t wait_node;
    list_node_t all_node;

    tss_t tss;
    int tss_sel;
}task_t;

int task_init(task_t* task, const char* name, int flag, uint32_t entry, uint32_t esp);
// 从from任务切换至to任务
void task_switch_from_to(task_t* from, task_t* to);

typedef struct _task_manager_t {
    task_t* curr_task;

    list_t ready_list;
    list_t task_list;
    list_t sleep_list;

    task_t first_task;
    task_t idle_task;

    int app_code_sel;
    int app_data_sel;
} task_manager_t;

void task_manager_init(void);
void task_first_init(void);
task_t* task_first_task(void);
void task_set_ready(task_t* task);
void task_set_block(task_t* task);
int sys_yield(void);
task_t * task_current(void);
void task_timer_tick(void);

file_t * task_file (int fd);
int task_alloc_fd (file_t * file);
void task_remove_fd (int fd);
void sys_exit(int status);
int sys_wait(int* status);

void task_set_sleep(task_t* task, uint32_t ticks);
void task_set_wakeup(task_t* task);
void sys_sleep(uint32_t ms);
void task_dispatch(void);
int sys_getpid (void);
int sys_fork (void);
int sys_execve(char *name, char * const *argv, char * const *env);
#endif