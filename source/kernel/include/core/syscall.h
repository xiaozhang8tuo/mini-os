#ifndef OS_SYSCALL_H
#define OS_SYSCALL_H

#define SYSCALL_PARAM_COUNT     5       	// 系统调用最大支持的参数

#define SYS_msleep              0
#define SYS_getpid              1
#define SYS_fork				2
#define SYS_execve              3
#define SYS_yield         		4

#define SYS_open                50
#define SYS_read                51
#define SYS_write               52
#define SYS_close               53
#define SYS_lseek				54
#define SYS_isatty              55
#define SYS_sbrk                56
#define SYS_fstat               57
#define SYS_dup              	58


#define SYS_printmsg            100
/**
 * 系统调用的栈信息
 */
typedef struct _syscall_frame_t {
	int eflags;
	int gs, fs, es, ds;
	int edi, esi, ebp, dummy, ebx, edx, ecx, eax; // pusha 是一种方便的方式，可以一次性保存所有的通用寄存器，其压栈顺序是 eax->... edi
	int eip, cs; // x86 架构的中断或异常处理过程中, %eip（指令指针寄存器）和 %cs（代码段寄存器）并不会被 pusha 指令压入栈中。它们是通过硬件自动保存和恢复的。
	int func_id, arg0, arg1, arg2, arg3;
	int esp, ss;
}syscall_frame_t;

void exception_handler_syscall (void);		// syscall处理

#endif //OS_SYSCALL_H