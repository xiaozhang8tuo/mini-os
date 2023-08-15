# 框架 #

![框架图](./mini-os/arch/arch.svg)


# 调试过程 #

通过指令`qemu` ` view` `compatmonitor0` : `info registers` 查看进入保护模式后各个**段选择子**的状态，以及**GDT**

![image-20230320001606641](.assets/image-20230320001606641.png)

`info mem` 查看内存地址的映射

![image-20230802164601950](.assets/image-20230802164601950.png)



线性地址0x80000000 

![image-20230802172745956](.assets/image-20230802172745956.png)

任务切换时的tss结构

![image-20230806205652656](.assets/image-20230806205652656.png)

两任务切换

![image-20230808224702814](.assets/image-20230808224702814.png)

# 问题解决记录 #

1 debug时配置的类型cppdbg不受支持，重启vscodec/c++扩展

2 VSCode调试debug变量variables窗口空白不显示

![image-20230813114802059](.assets/image-20230813114802059.png)

3 反汇编查看内核运行指令![image-20230813223431768](.assets/image-20230813223431768.png)

# Q&A #

## 0 保护模式是怎么进入的 ##

mov $1, %eax    lmsw %ax      # 设置CR0  P/E 位, 开启保护模式。在这之前要把，两个最重要的寄存器设置好

```cpp
    cli                # 关中断
    lgdt (gdt_desc)    # 设置lgdt寄存器
    lidt (idt_desc)    # 设置igdt寄存器
    mov $1, %eax
    lmsw %ax          # 设置CR0 P/E 位, 开启保护模式
```

## 0.1 内核初始化做了什么 ##

1 初始化8259中断控制器，打开定时器中断 

2 设置定时器，每100ms中断一次 | 添加中断，设置第0x20号中断 **idt表**

3 添加任务和系统调用 **gdt表**

4 设置页表

5 ltr：将任务0的TSS段选择符加载到任务寄存器tr，只明确加这一次

## 0.2 页表相关的操作有什么寄存器 ##

打开分页机制,设置页表项  cr0(开启分页) cr3(设置页表) cr4(支持4MB直接映射)寄存器

## 1 8259定时器的调度是怎么实现的？ ##

通过cli,sti关开中断，idt表中中断处理函数的0x20被设定为中断处理函数

http://www.xjbcode.fun/Notes/001-modern-computing/008-80286-interrupt-gate.html 

非屏蔽中断的向量和异常的向量是固定的，而屏蔽中断的向量可以通过对中断控制器的编程来改变。但通常大家都遵循一个惯例。比如Linux 对256 个向量的分配如下。 • 从0~31 的向量对应于**异常**和**非屏蔽中断**。 • 从32~47 的向量（即由I/O 设备引起的中断，共16个，对应两个8259A级联所通产生的最大中断个数）分配给可屏蔽中断。32——47也即**0x20~0x2F**。

## 2 任务的调度怎么实现的 ##

在定时处理函数timer_int中，每次都会执行task_sched函数(在内核态中)，任务切换**策略为交替执行**，ljmp 到任务的tss段

```cpp
/*
	task_sched()将切换任务
 */
// 跳转到一个任务的TSS段选择符组成的地址处会造成CPU进行任务切换操作。
// 其中临时数据结构addr用于组建远跳转(far jump)ljmp指令的操作数。该操作数由4字节偏移
// 地址和2字节的段选择符组成。因此tmp中0的值是32位偏移值，而低2字节是新TSS段的
// 选择符task_tss（高2字节不用）。跳转到TSS段选择符会造成任务切换到该TSS对应的进程。对于造成任务
// 切换的长跳转
// 其格式为：jmp 16位段选择符:32位偏移值。但在内存中操作数的表示顺序与这里正好相反。       
void task_sched (void) {
    static int task_tss = TASK0_TSS_SEG;

    // 更换当前任务的tss，然后切换过去
    task_tss = (task_tss == TASK0_TSS_SEG) ? TASK1_TSS_SEG : TASK0_TSS_SEG;
    uint32_t addr[] = {0, task_tss };
    __asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr));
}
```

## 3 完整的一次系统调用 ##

1 用户态发起系统调用

```cpp
void sys_show(char* str, char color)
{
    const unsigned long sys_gate_addr[] = {0, SYS_CALL_SEG};  // 使用特权级0
    __asm__ __volatile__("push %[color];    push %[str];      push %[id];       lcalll *(%[gate])\n\n "
            ::[color]"m"(color), [str]"m"(str), [id]"r"(2), [gate]"r"(sys_gate_addr));
}
```

2 调用系统调用表先前已经注册好的系统调用处理函数

```cpp
gdt_table[SYS_CALL_SEG/ 8].limit_l = (uint16_t)(uint32_t)syscall_handler;
```

3 保护现场，把参数依次压入栈中，**call**调用真正的do_syscall

```assembly
syscall_handler:
    push %ds
	pusha						# 保护现场，段寄存器不用保存
	mov $KERNEL_DATA_SEG, %ax
	mov %ax, %ds				#  Push AX, CX, DX, BX, original SP, BP, SI, and DI.

    mov %esp, %ebp				# 下面压栈时,esp会不断变化,所以使用ebp作为基址
	push 13*4(%ebp)				# 提取出原压入的各项参数,再按相同的顺序压入一遍
	push 12*4(%ebp)
	push 11*4(%ebp)
	call do_syscall				# 调用api函数,注意这时是在特级0的模式下运行,可以做很多有需要高权限才能做的事
    add $(3*4), %esp			# 取消刚才压入的值
	popa						# 恢复现场
	pop %ds
	retf $(3*4)					# 使用远跳转
```
## 4 iret和retf的区别 ##
iret和retf都是汇编语言中的跳转指令，用于从子程序返回到调用它的位置。它们的主要区别在于它们所处理的中断类型不同。 

iret指令用于从中断服务例程返回，它会从堆栈中弹出标志寄存器（flags）、代码段选择子（cs）和指令指针（ip），然后继续执行主程序。 大多是时钟中断等

retf指令用于从远程过程调用（RPC）或任务切换返回，它会从堆栈中弹出标志寄存器（flags）、代码段选择子（cs）、指令指针（ip）和堆栈选择子（ss），然后继续执行主程序。 用户态调用内核态函数嘞，用户的栈ss
要保存着

总的来说，iret用于从中断返回，而retf用于从远程过程调用或任务切换返回。

## 5 gdb attach ##

在GDB中，"attach" 是一种将调试器连接到正在运行的进程的操作。这允许你在进程运行时进行调试，包括设置断点、检查变量值、观察堆栈等。"attach" 操作的原理涉及以下几个步骤：

1. **进程附加：** 当你使用GDB的 "attach" 命令时，GDB会尝试连接到指定的进程，它会向目标进程发送一些控制信息以建立连接。
2. **注入调试代码：** 在成功连接到进程后，GDB会在目标进程的内存空间中注入一些调试代码，这些代码将用于与调试器通信，收集信息以及实现调试功能。
3. **设置断点：** 一旦连接建立，你可以在目标进程中设置断点。GDB会在你指定的断点位置插入调试代码，当程序执行到这些位置时，会触发一个中断信号。
4. **中断信号和处理：** 当目标进程达到断点位置时，注入的调试代码将触发一个中断信号，即"中断"。这个信号会暂停目标进程的执行，并将控制权交给GDB调试器。
5. **与调试器交互：** 一旦目标进程被中断，GDB调试器会接管执行控制。你可以使用GDB的命令来检查变量、观察堆栈、单步执行等。你还可以修改变量值，修改寄存器值，然后使用GDB的命令来继续执行程序。

总之，"attach" 操作允许GDB连接到正在运行的进程，通过注入调试代码和中断信号，以便在运行时进行调试。这对于调试已经在运行的进程、在嵌入式系统中调试或者远程调试等场景非常有用。

## 6 boot到loader是怎么跳转的 ##

直接通过内存地址强转的硬跳转

```cpp
#define LOADER_START_ADDR 0X8000

/**
 * Boot的C入口函数
 * 只完成一项功能，即从磁盘找到loader文件然后加载到内容中，并跳转过去
 */
void boot_entry(void) {
    ( (void(*)(void)) LOADER_START_ADDR )();    //强转地址为函数指针做跳转
} 
```



# OS #

![image-20230813211151879](.assets/image-20230813211151879.png)

x /20xb 0x8000：访问0x8000后面20个字节(b)，以16(x)进制形式显示

![image-20230813211226264](.assets/image-20230813211226264.png)