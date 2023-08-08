#include "os.h"

// 
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;


void syscall_handler(void);

void do_syscall (int func, char *str, char color) {
    static int row = 1;     // 初始值不能为0，否则其初始化值不确定

    if (func == 2) {
        // 显示器共80列，25行，按字符显示，每个字符需要用两个字节表示
        unsigned short * dest = (unsigned short *)0xb8000 + 80*row;
        while (*str) {
            // 其中一个字节保存要显示的字符，另一个字节表示颜色
            *dest++ = *str++ | (color << 8);
        }

        // 逐行显示，超过一行则回到第0行再显示
        row = (row >= 25) ? 0 : row + 1;

        // 加点延时，让显示慢下来
        for (int i = 0; i < 0xFFFFFF; i++) ;
    }
}

void sys_show(char* str, char color)
{
    // do_syscall(2, str, color);
    const unsigned long sys_gate_addr[] = {0, SYS_CALL_SEG};  // 使用特权级0

    // 采用调用门, 这里只支持5个参数
    // 用调用门的好处是会自动将参数复制到内核栈中，这样内核代码很好取参数
    // 而如果采用寄存器传递，取参比较困难，需要先压栈再取
    __asm__ __volatile__("push %[color];    push %[str];      push %[id];       lcalll *(%[gate])\n\n "
            ::[color]"m"(color), [str]"m"(str), [id]"r"(2), [gate]"r"(sys_gate_addr));
}

void task0()
{
    char * str = "task b: 1234";
    uint8_t color = 0xff;
    for (;;)
    {
        sys_show(str, color++);
    }
}

void task1()
{
    char * str = "task a: 5678";
    uint8_t color = 0;
    for (;;)
    {
        sys_show(str, color--);
    }
}



#define MAP_ADDR        (0x80000000)            // 要映射的地址
// 设置页表项 PS: 这里根据手册32-bit page，不启用二级页表, 只使用1级页表映射物理内存就好
#define PDE_P  (1 << 0)
#define PDE_W  (1 << 1)
#define PDE_U  (1 << 2)
#define PDE_PS (1 << 7)

/**
 * @brief 任务0和1的栈空间
 */
uint32_t task0_dpl3_stack[1024];

uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))) = {0x36};

static uint32_t pg_table[1024] __attribute__((aligned(4096))) = {PDE_U};    // 要给个值，否则其实始化值不确定

uint32_t pg_dir[1024] __attribute__((aligned(4096))) = {
    [0] = (0) | PDE_P | PDE_PS | PDE_W | PDE_U,	    // PDE_PS，开启4MB的页，恒等映射
};


/**
 * @brief 任务0和1的栈空间
 */
uint32_t task0_dpl0_stack[1024], task0_dpl3_stack[1024], task1_dpl0_stack[1024], task1_dpl3_stack[1024];

/**
 * @brief 任务0的任务状态段
 */
uint32_t task0_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,  (uint32_t)task0_dpl0_stack + 4*1024, KERNEL_DATA_SEG , /* 后边不用使用 */ 0x0, 0x0, 0x0, 0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)pg_dir,  (uint32_t)task0/*入口地址*/, 0x202, 0xa, 0xc, 0xd, 0xb, (uint32_t)task0_dpl3_stack + 4*1024/* 栈 */, 0x1, 0x2, 0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    APP_DATA_SEG, APP_CODE_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, 0x0, 0x0,
};

uint32_t task1_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,  (uint32_t)task1_dpl0_stack + 4*1024, KERNEL_DATA_SEG , /* 后边不用使用 */ 0x0, 0x0, 0x0, 0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)pg_dir,  (uint32_t)task1/*入口地址*/, 0x202, 0xa, 0xc, 0xd, 0xb, (uint32_t)task1_dpl3_stack + 4*1024/* 栈 */, 0x1, 0x2, 0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    APP_DATA_SEG, APP_CODE_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, 0x0, 0x0,
};


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

// idt表中的中断处理函数
struct {uint16_t offset_l, selector, attr, offset_h;} idt_table[256] __attribute__((aligned(8))) = {1};


// GDT表中的数据段和代码段设置
struct { uint16_t limit_l, base_l, basehl_attr, base_limit;} gdt_table[256] __attribute__((aligned(8))) = {
    // 0x00cf9a000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，32位代码段（非一致代码段），界限4G，
    [KERNEL_CODE_SEG/8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
    // 0x00cf93000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，数据段，界限4G，可读写
    [KERNEL_DATA_SEG/8] = {0xffff, 0x0000, 0x9200, 0x00cf},
    // 0x00cffa000000ffff - 从0地址开始，P存在，DPL=3，Type=非系统段，32位代码段，界限4G
    [APP_CODE_SEG/ 8] = {0xffff, 0x0000, 0xfa00, 0x00cf},
    // 0x00cff3000000ffff - 从0地址开始，P存在，DPL=3，Type=非系统段，数据段，界限4G，可读写
    [APP_DATA_SEG/ 8] = {0xffff, 0x0000, 0xf300, 0x00cf},

    // 两个进程的task0和tas1的tss段:自己设置，直接写会编译报错
    [TASK0_TSS_SEG/ 8] = {0x0068, 0, 0xe900, 0x0},
    [TASK1_TSS_SEG/ 8] = {0x0068, 0, 0xe900, 0x0},

    // 系统调用的调用门, 在init中初始化
    [SYS_CALL_SEG / 8] = {0x0000, KERNEL_CODE_SEG, 0xec03, 0x0000},
};


// 反汇编结果
// __asm__ __volatile__("outb %[v], %[p]"::[p]"d"(port), [v]"a"(data));
// 7e54:	0f b7 55 f8          	movzwl -0x8(%ebp),%edx
// 7e58:	0f b6 45 fc          	movzbl -0x4(%ebp),%eax
// 7e5c:	ee                   	out    %al,(%dx)
void outb(uint8_t data, uint16_t port)
{
    __asm__ __volatile__("outb %[v], %[p]"::[p]"d"(port), [v]"a"(data));
}


void timer_int (void);

void os_init (void) {
    // 初始化8259中断控制器，打开定时器中断
    outb(0x11, 0x20);  // 开始初始化主芯片       
    outb(0x11, 0xA0);  // 初始化从芯片   
    outb(0x20, 0x21);  // 写ICW2，告诉主芯片中断向量从0x20开始   
    outb(0x28, 0xA1);  // 写ICW2，告诉从芯片中断向量从0x28开始   
    outb(1<<2, 0x21);  // 写ICW3，告诉主芯片IRQ2上连接有从芯片   
    outb(2, 0xa1);     // 写ICW3，告诉从芯片连接g到主芯片的IRQ2上
    outb(0x1, 0x21);   // 写ICW4，告诉主芯片8086、普通EOI、非缓冲模式   
    outb(0x1, 0xA1);   // 写ICW4，告诉主芯片8086、普通EOI、非缓冲模式   
    outb(0xfe, 0x21);  // 开定时中断，其它屏幕   
    outb(0xff, 0xA1);  // 屏蔽所有中断   

    // 设置定时器，每100ms中断一次
    int tmo = (1193180);      // 时钟频率为1193180
    outb(0x36, 0x43);               // 二进制计数、模式3、通道0
    outb((uint8_t)tmo, 0x40);
    outb(tmo >> 8, 0x40);

    // 添加中断，设置第0x20号中断
    idt_table[0x20].offset_h = (uint32_t)timer_int >> 16;
    idt_table[0x20].offset_l = (uint32_t)timer_int & 0xffff;
    idt_table[0x20].selector = KERNEL_CODE_SEG;
    idt_table[0x20].attr = 0x8E00;      // 存在，DPL=0, 中断门

    // 添加任务和系统调用
    gdt_table[TASK0_TSS_SEG / 8].base_l = (uint16_t)(uint32_t)task0_tss;
    gdt_table[TASK1_TSS_SEG / 8].base_l = (uint16_t)(uint32_t)task1_tss;
    gdt_table[SYS_CALL_SEG/ 8].limit_l = (uint16_t)(uint32_t)syscall_handler;

    // 页目录表的第一项做恒等映射，不对应二级页表
    // 页目录表的第512项指向对应的二级页表，二级页表对应map_phy_buffer的地址
    // 虚拟内存
    // 0x80000000开始的4MB区域的映射
    pg_dir[MAP_ADDR >> 22] = (uint32_t)pg_table | PDE_P | PDE_W | PDE_U;
    pg_table[(MAP_ADDR >> 12) & 0x3FF] = (uint32_t)map_phy_buffer| PDE_P | PDE_W | PDE_U;
};


