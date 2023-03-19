#include "os.h"

// 
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;

struct 
{
    uint16_t limit_l, base_l;
    uint16_t basehl_attr, base_limit;
}gdt_table[256] __attribute__((aligned(8))) = 

{
    // 0x00cf9a000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，32位代码段（非一致代码段），界限4G，
    [KERNEL_CODE_SEG/8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
    // 0x00cf93000000ffff - 从0地址开始，P存在，DPL=0，Type=非系统段，数据段，界限4G，可读写
    [KERNEL_DATA_SEG/8] = {0xffff, 0x0000, 0x9200, 0x00cf},
};