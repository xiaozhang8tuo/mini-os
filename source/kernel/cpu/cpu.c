#include "cpu/cpu.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE];

void segment_desc_set (int selector, uint32_t base, uint32_t limit, uint32_t attr) {
    segment_desc_t* desc = gdt_table + (selector >> 3);

    if (limit > 0xFFFFF) {
        attr |= SEG_G;
        // 如果limit界限太大，就变换单位，看看有多少个4KB(12位)  limit本身20位，4KB(12位)。所以limit最大界限32位，即0xFFFFFFFF
        limit /= 0X1000;
    }

    desc->limit15_0 = limit & 0xFFFF;
    desc->base15_0 = base & 0xFFFF;
    desc->base23_16 = (base >> 16) & 0xFF;
    desc->attr = attr | (((limit >> 16) & 0xF) << 8);
    desc->base31_24 = (base >> 24) & 0XFF;
}

void init_gdt (void) {
    for (int i=0; i< GDT_TABLE_SIZE; i++) {
        segment_desc_set(i << 3, 0, 0, 0);
    }

    segment_desc_set(KERNEL_SELECTOR_CS, 0, 0xFFFFFFFF, );
    segment_desc_set(KERNEL_SELECTOR_DS, 0, 0xFFFFFFFF, );

    lgdt((uint32_t)gdt_table, sizeof(gdt_table));

}

void cpu_init (void) {
    init_gdt();
}