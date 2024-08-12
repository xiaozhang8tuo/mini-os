#include "cpu/cpu.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE];
static mutex_t mutex;

/*设置段描述符*/
void segment_desc_set (int selector, uint32_t base, uint32_t limit, uint16_t attr) {
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

/*初始化 中断向量表表项*/
void gate_desc_set (gate_desc_t* desc, uint16_t selector, uint32_t offset, uint16_t attr) {
    desc->offset15_0 = offset & 0xFFFF;
    desc->selector = selector;
    desc->attr = attr;
    desc->offset31_16 = (offset >> 16) & 0xFFFF;
}

int gdt_alloc_desc() {
    // irq_state_t state = irq_enter_protection();
    mutex_lock(&mutex);
    for (int i=1; i < GDT_TABLE_SIZE; i++) {
        segment_desc_t* desc = gdt_table + i;
        if (desc->attr == 0) {
            // irq_leave_protection(state);
            mutex_unlock(&mutex);
            return i * sizeof(segment_desc_t);
        }
    }
    // irq_leave_protection(state);
    mutex_unlock(&mutex);
    return -1;
}

void gdt_free_sel(int sel) {
    mutex_lock(&mutex);
    gdt_table[sel/sizeof(segment_desc_t)].attr = 0;
    mutex_unlock(&mutex);
}

void init_gdt (void) {
    for (int i=0; i< GDT_TABLE_SIZE; i++) {
        segment_desc_set(i << 3, 0, 0, 0);
    }

    segment_desc_set(KERNEL_SELECTOR_CS, 0, 0xFFFFFFFF, 
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);
    segment_desc_set(KERNEL_SELECTOR_DS, 0, 0xFFFFFFFF, 
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);

    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
    // 调用 lgdt 指令是加载全局描述符表（GDT）到CPU的GDTR寄存器中。
    // lgdt 指令本身只负责更新GDTR寄存器，它不涉及更改当前活动的段寄存器（如CS、DS、ES等）。
    // 因此，一旦新的GDT被加载，你需要执行额外的步骤即(gdt_reload)来告诉CPU使用新的GDT中的段描述符
}

void cpu_init (void) {
    init_gdt();
    mutex_init(&mutex);
}

void switch_to_tss (int tss_sel) {
    far_jump(tss_sel, 0);
}
