# 调试过程 #

通过指令`qemu` ` view` `compatmonitor0` : `info registers` 查看进入保护模式后各个**段选择子**的状态，以及**GDT**

![image-20230320001606641](.assets/image-20230320001606641.png)

`info mem` 查看内存地址的映射

![image-20230802164601950](.assets/image-20230802164601950.png)



线性地址0x80000000 

![image-20230802172745956](.assets/image-20230802172745956.png)

任务切换时的tss结构

![image-20230806205652656](.assets/image-20230806205652656.png)



# 问题解决记录 #

1 debug时配置的类型cppdbg不受支持，重启vscodec/c++扩展

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

