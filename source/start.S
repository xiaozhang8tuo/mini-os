#include "os.h"
# 声明本地以下符号是全局的，在其它源文件中可以访问
.global _start

# 指定以下的代码生成16位的机器指令，这样才能在启动时的实模式下运行
.code16

.text

_start:
    # set ds es ss and sp  
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %gs
    mov %ax, %fs
    mov $_start, %esp


# bios只把512B加载了，512B后面的并没有加载，需要我们手动加载
read_self_all:
    # mov $0x7e00, %bx   # buffer addr 0x7c00+512=0x7e00        // start address 0x00007c00 反汇编文件中可以看到 7c00就是程序的起始地址
    mov $_start_32, %bx  # buffer addr 0x7c00+512=0x7e00  
    mov $0x2,    %cx   # cl:start sect
    mov $0x0240, %ax   # ah-->0x02: read; al-->0x40=64: 64 nr of sects 64*512B
    mov $0x80,   %dx   # what's device 0x80 is the first disk 
    int $0x13          # read 64 sects in first disk start from 2 sect to buffer addr 
    jc  read_self_all  # after read disk, check the cf to judge read success? if not jmp to continue read 

    cli                # 关中断
    lgdt (gdt_desc)    # 设置lgdt寄存器
    mov $1, %eax
    lmsw %ax          # 设置CR0 P/E 位, 开启保护模式
    # cs寄存器设置为KERNEL_CODE_SEG(段选择子索引为1), 因为起始地址是0, 加_start_32偏移量，还是跳到_start_32的位置
    jmp $KERNEL_CODE_SEG, $_start_32 



    .org 0x1fe # 510
    .byte 0x55, 0xaa # alloc   unsigned char s[] = {0x55,0xaa}; in 510 boot flag 
.code32     # 保护模式32位
.text
_start_32:
    # .fill 64*1024, 1, 0x35 # fill repeat, size, val     // gdb 中查看内存分布 -exec x /32x 0x7e00 
    mov $KERNEL_DATA_SEG, %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %gs
    mov %ax, %fs
    jmp .



gdt_desc:
    .word (256*8) - 1
    .long gdt_table