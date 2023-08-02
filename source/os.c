#include "os.h"

// 
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;


// 设置页表项 PS: 这里根据手册32-bit page，不启用二级页表, 只使用1级页表映射物理内存就好
#define PDE_P  (1 << 0)
#define PDE_W  (1 << 1)
#define PDE_U  (1 << 2)
#define PDE_PS (1 << 7)

uint32_t pg_dir[1024] __attribute__((aligned(4096))) = {
    [0] = (0) | PDE_P | PDE_PS | PDE_W | PDE_U,	    // PDE_PS，开启4MB的页，恒等映射
};


// GDT表中的数据段和代码段设置
struct { uint16_t limit_l, base_l, basehl_attr, base_limit;} gdt_table[256] __attribute__((aligned(8))) = {
    // 0x00cf9a000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，32位代码段（非一致代码段），界限4G，
    [KERNEL_CODE_SEG/8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
    // 0x00cf93000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，数据段，界限4G，可读写
    [KERNEL_DATA_SEG/8] = {0xffff, 0x0000, 0x9200, 0x00cf},
};