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

.macro exception_handler name num with_error_code
    .text
    .extern do_handler_\name
    .global exception_handler_\name
exception_handler_\name:
    .if \with_error_code == 0
        push $0
    .endif

    push $\num
    # 保存所有寄存器
    pusha # push general-purpose registers Figure 3-4
    push %ds
    push %es
    push %fs
    push %gs
    
    # 调用中断处理函数
    push %esp
    call do_handler_\name
    add $(1*4), %esp # 丢掉esp
    
    # 恢复保存的寄存器
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    add $(2*4), %esp # 丢掉 num 和 error_code

    iret
.endm

exception_handler unknow, -1, 0
exception_handler division, 0, 0
exception_handler Debug, 1, 0
exception_handler NMI, 2, 0
exception_handler breakpoint, 3, 0
exception_handler overflow, 4, 0
exception_handler bound_range, 5, 0
exception_handler invalid_opcode, 6, 0
exception_handler device_unavailable, 7, 0
exception_handler double_fault, 8, 1
exception_handler invalid_tss, 10, 1
exception_handler segment_not_present, 11, 1
exception_handler stack_segment_fault, 12, 1
exception_handler general_protection, 13, 1
exception_handler page_fault, 14, 1
exception_handler fpu_error, 16, 0
exception_handler alignment_check, 17, 1
exception_handler machine_check, 18, 0
exception_handler simd_exception, 19, 0
exception_handler virtual_exception, 20, 0

exception_handler timer, 0x20, 0