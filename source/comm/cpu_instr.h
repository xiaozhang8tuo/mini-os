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

static inline uint16_t inw(uint16_t  port) {
	uint16_t rv;
	__asm__ __volatile__("in %1, %0" : "=a" (rv) : "dN" (port));
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
// 汇编指令字符串："mov %%cr0, %[v]"

// mov 是汇编语言中的移动指令，用于将一个值从源操作数传送到目标操作数。
// %%cr0 是对CR0控制寄存器的引用。在内嵌汇编中，%% 用于转义 % 符号，因为 % 在格式字符串中用作特殊字符。
// %[v] 是一个操作数占位符，它将在后面的约束中被具体化，指向某个变量或寄存器。
// 约束修饰符：[v]"=r"(cr0)

// [v] 是与汇编指令字符串中的 %[v] 相匹配的命名操作数占位符。
// 等号 = 表示这是一个输出操作数，汇编指令将向其写入数据。
// r 表示这个输出操作数应该映射到任意一个通用寄存器上。
// cr0 是C语言中的变量，它将接收汇编指令的输出结果，即CR0寄存器的值。
// 因此，整个表达式的意思是在汇编层面上执行 mov %%cr0, %[v] 指令，并将结果存储在C语言变量 cr0 所对应的寄存器中。编译器会根据约束修饰符 "=r" 来选择一个合适的寄存器，并在汇编代码执行后确保 cr0 变量包含CR0寄存器的值。

// 第一个 : 后面跟随的是输出操作数，而第二个 : 后面跟随的是输入操作数。如果某个部分不存在，可以省略相应的 :
static inline void write_cr0(uint32_t v) {
	__asm__ __volatile__("mov %[v], %%cr0"::[v]"r"(v));
}

static inline void far_jump(uint32_t selector, uint32_t offset) {
	uint32_t addr[] = {offset, selector };
	__asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr));
}


#endif