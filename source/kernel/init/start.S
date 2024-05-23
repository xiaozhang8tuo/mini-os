#include "os_cfg.h"
    .text
    .extern kernel_init
    .global _start
    .extern init_main
    # void start (boot_info_t* boot_info)
_start:
    # 参照test是怎么取参数的
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax # 此时eax中的值，应该就是boot_info的地址，可以去loader_elf.txt 中查看boot_info的位置
    # 以上三句可替换为 mov 0x4(%esp), %eax   因为在进入函数的时候，esp指向的是返回地址，上面就是传入的参数

    # 参照是怎么，压栈参数，发起调用的
    # kernel_init(boot_info)
    push %eax
    call kernel_init
	# 重新加载GDT
    # jmp 是汇编语言中的无条件跳转指令，它告诉CPU跳转到指定的标签或地址继续执行指令
    # $KERNEL_SELECTOR_CS 是一个立即数，它指定了内核代码段的选择器
    # 在x86架构中，段选择器是一个20位的值，用于在GDT中索引一个段描述符。这个值告诉CPU应该使用哪个段作为代码段
    # $gdt_reload 是代码中的一个标签，指向新的GDT设置完毕后继续执行的位置
	jmp $KERNEL_SELECTOR_CS, $gdt_reload

gdt_reload:
	mov $KERNEL_SELECTOR_DS, %ax		# 16为数据段选择子
	mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

	# 栈设置
	mov $(stack + KERNEL_STACK_SIZE), %esp

	# 栈和段等沿用之前的设置
	jmp init_main

	.bss
.comm stack, KERNEL_STACK_SIZE    # comm 声明未初始化的通用内存区域，以字节计