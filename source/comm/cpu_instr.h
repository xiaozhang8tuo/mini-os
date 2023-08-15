#ifndef CPU_INSTR_H
#define CPU_INSTR_H

// inb 用于读取对应port中的字节
static inline uint8_t inb(uint16_t port)
{
    uint8_t rv;

    __asm__ __volatile__("inb %[port], %[v]  \n\t"
                         :[v]"=a"(rv)
                         :[port]"d"(port)
                         : );
    return rv;
} 

// outb 用于将一个字节（8位数据）写入指定的I/O端口
static inline void outb(uint16_t port, uint8_t data) {
	__asm__ __volatile__("outb %[v], %[p]" 
                        : 
                        : [p]"d" (port), [v]"a" (data));
}

static inline void sti(void)
{
    __asm__ __volatile__("sti");
}

static inline void cli(void)
{
    __asm__ __volatile__("cli");
}

static inline void lgdt(uint32_t start, uint32_t size)
{
    struct gdt
    {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } gdt;

    // gdt_desc:               # GDTR寄存器格式
    // .word (256*8) - 1   # 16位界限  
    // .long gdt_table     # 32位基地址

    gdt.limit = size - 1;
    gdt.start15_0  = start & 0xffff;
    gdt.start31_16 = start>>16;

    __asm__ __volatile__ ("lgdt %[g]" ::[g]"m"(gdt));
}

static inline uint32_t read_cr0() {
	uint32_t cr0;
	__asm__ __volatile__("mov %%cr0, %[v]":[v]"=r"(cr0));
	return cr0;
}

static inline void write_cr0(uint32_t v) {
	__asm__ __volatile__("mov %[v], %%cr0"::[v]"r"(v));
}

static inline void far_jump(uint32_t selector, uint32_t offset) {
	uint32_t addr[] = {offset, selector };
	__asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr));
}


#endif