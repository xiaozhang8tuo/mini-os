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

read_self_all:
    # mov $0x7e00, %bx   # buffer addr 0x7c00+512=0x7e00
    mov $_start_32, %bx  # buffer addr 0x7c00+512=0x7e00  
    mov $0x2,    %cx   # cl:start sect
    mov $0x0240, %ax   # ah-->0x02: read; al-->0x40=64: 64 nr of sects 64*512B
    mov $0x80,   %dx   # what's device 0x80 is the first disk 
    int $0x13          # read 64 sects in first disk start from 2 sect to buffer addr 
    jc  read_self_all  # after read disk, check the cf to judge read success? if not jmp to continue read 


    jmp .

    .org 0x1fe # 510
    .byte 0x55, 0xaa # alloc   unsigned char s[] = {0x55,0xaa}; in 510 boot flag 
_start_32:
    .fill 64*1024, 1, 0x35 # fill repeat, size, val