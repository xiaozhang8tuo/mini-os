#include "os.h"
# 声明本地以下符号是全局的，在其它源文件中可以访问
.global _start, timer_int

# 指定以下的代码生成16位的机器指令，这样才能在启动时的实模式下运行
.code16

.text

_start:
    # set ds es ss and sp  
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %gs
    mov %ax, %fs
    mov $_start, %esp


# bios 只把磁盘中的第一个扇区(0xaa,0x55结尾)，加载到内存中(起始地址0x7c00后面的512B)
# 512B 后面的并没有加载到内存中，需要我们手动加载
read_self_all:
    # mov $0x7e00, %bx   # buffer addr 0x7c00+512=0x7e00        // start address 0x00007c00 反汇编文件中可以看到 7c00就是程序的起始地址
    mov $_start_32, %bx  # buffer addr 0x7c00+512=0x7e00  
    mov $0x2,    %cx   # cl:start sect
    mov $0x0240, %ax   # ah-->0x02: read; al-->0x40=64: 64 nr of sects 64*512B
    mov $0x80,   %dx   # what's device 0x80 is the first disk 
    int $0x13          # read 64 sects in first disk start from 2 sect to buffer addr 
    jc  read_self_all  # after read disk, check the cf to judge read success? if not jmp to continue read 


    # 进入保护模式
    cli                # 关中断
    lgdt (gdt_desc)    # 设置lgdt寄存器
    lidt (idt_desc)    # 设置igdt寄存器
    mov $1, %eax
    lmsw %ax          # 设置CR0 P/E 位, 开启保护模式
    # cs寄存器设置为KERNEL_CODE_SEG(段选择子索引为1), 因为起始地址是0, 加_start_32偏移量，还是跳到_start_32的位置
    jmp $KERNEL_CODE_SEG, $_start_32 



# 跳到引导标志, 由bios在上电后检查
.org 0x1fe # 510  // 引导标志
.byte 0x55, 0xaa # alloc   unsigned char s[] = {0x55,0xaa}; in 510 boot flag 

.code32     # 保护模式32位
.text
_start_32:
    # .fill 64*1024, 1, 0x35 # fill repeat, size, val     // gdb 中查看内存分布 -exec x /32x 0x7e00 
    mov $KERNEL_DATA_SEG, %eax  # 设置数据段相关的段选择子
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %gs
    mov %ax, %fs

    mov $_start, %esp

    call os_init

    # 打开分页机制,设置页表项  cr0(开启分页) cr3(设置页表) cr4(支持4MB直接映射)寄存器
	mov $pg_dir, %eax
	mov %eax, %cr3

    # 使用1级页表，支持4MB 
	mov %cr4, %eax
	orl $(1 << 4), %eax		    # PSE位,支持4MB分页
	mov %eax, %cr4

    # 开启分页机制,设置cr0中的标志位
	mov %cr0, %eax
	orl $(1 << 31), %eax		# 打开PG位,开启分页机制
	mov %eax, %cr0              # 执行完毕这一步之后，0x80000000(线性地址)才通过分页机制映射到了对应变量


    mov $TASK0_TSS_SEG, %ax
    ltr %ax                     # 将任务0的TSS段选择符加载到任务寄存器tr。只明确加这一次，以后新任务LDT的加截，是CPU根据TSS中的LDT项自动加载。

	# 下面模拟中断返回，从而实现从特权级0到特权级3的变化
	# 中断发生时，会自动压入原SS, ESP, EFLAGS, CS, EIP到栈中
	push $APP_DATA_SEG
	push $task0_dpl3_stack + 1024	# 特权级3时的栈, 
	push $0x202			# 中断暂时关掉 0x202						# EFLAGS 标值寄存器的中断允许IF置位
	push $APP_CODE_SEG				# CPL=3时的选择子
	push $task_0_entry					# eip
	iret							# 从中断返回，将切换至任务0



task_0_entry:
	# 进入任务0时,需要重设其数据段寄存器为特权级3的
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	jmp task0


# 定时器0中断函数
# 这段代码并未直接调用IDT表中的中断处理程序，而是执行了中断处理函数的一些常规步骤，
# 然后通过iret指令返回到中断发生前的现场，从而继续执行之前的程序。在实际场景中，中断处理程序会在IDT表中注册，并在需要时由CPU调用。
timer_int:
	push %ds
	pusha						# 保护现场，段寄存器不用保存

	mov $0x20, %al              # !!!注意这里的0x20是表示中断结束(EOI END OF INTER)的特殊值和8259的口无关
	outb %al, $0x20				# 发送EOI(0x20) 到 8259的recv口


	# 使用内核的数据段寄存器，因为要访问内核的数据 !!!
	mov $KERNEL_DATA_SEG, %ax
	mov %ax, %ds

    call task_sched             # 在定时器这里执行任务切换

	popa						# 恢复现场
	pop %ds
	iret						# 中断返回


gdt_desc:               # GDTR寄存器格式
    .word (256*8) - 1   # 16位界限  
    .long gdt_table     # 32位基地址


idt_desc:               # IDTR寄存器格式
    .word (256*8) - 1   # 16位界限  
    .long idt_table     # 32位基地址