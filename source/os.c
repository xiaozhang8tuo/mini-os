#include "os.h"

// 
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;

#define MAP_ADDR        (0x80000000)            // 要映射的地址
// 设置页表项 PS: 这里根据手册32-bit page，不启用二级页表, 只使用1级页表映射物理内存就好
#define PDE_P  (1 << 0)
#define PDE_W  (1 << 1)
#define PDE_U  (1 << 2)
#define PDE_PS (1 << 7)

uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))) = {0x36};

static uint32_t pg_table[1024] __attribute__((aligned(4096))) = {PDE_U};    // 要给个值，否则其实始化值不确定

uint32_t pg_dir[1024] __attribute__((aligned(4096))) = {
    [0] = (0) | PDE_P | PDE_PS | PDE_W | PDE_U,	    // PDE_PS，开启4MB的页，恒等映射
};


// idt表中的中断处理函数
struct {uint16_t offset_l, selector, attr, offset_h;} idt_table[256] __attribute__((aligned(8))) = {1};


// GDT表中的数据段和代码段设置
struct { uint16_t limit_l, base_l, basehl_attr, base_limit;} gdt_table[256] __attribute__((aligned(8))) = {
    // 0x00cf9a000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，32位代码段（非一致代码段），界限4G，
    [KERNEL_CODE_SEG/8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
    // 0x00cf93000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，数据段，界限4G，可读写
    [KERNEL_DATA_SEG/8] = {0xffff, 0x0000, 0x9200, 0x00cf},
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

    // 页目录表的第一项做恒等映射，不对应二级页表
    // 页目录表的第512项指向对应的二级页表，二级页表对应map_phy_buffer的地址
    // 虚拟内存
    // 0x80000000开始的4MB区域的映射
    pg_dir[MAP_ADDR >> 22] = (uint32_t)pg_table | PDE_P | PDE_W | PDE_U;
    pg_table[(MAP_ADDR >> 12) & 0x3FF] = (uint32_t)map_phy_buffer| PDE_P | PDE_W | PDE_U;
};


