#ifndef OS_SYSCALL_H
#define OS_SYSCALL_H

#define SYSCALL_PARAM_COUNT     5       	// 系统调用最大支持的参数

#define SYS_msleep              0

void exception_handler_syscall (void);		// syscall处理

#endif //OS_SYSCALL_H