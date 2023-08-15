boot+loader 引导程序

基础内核 支持多进程运行

带保护的内核 支持内存分页与进程保护

带shelle的内核 支持进程的动态加载与退出

带文件系统的内核 支持设备文件系统和FAT16文件系统

# 2-2 #

## 查看反汇编确定 _start_32的值0x7ce0 ##

```assembly
read_self_all:
    # mov $0x7e00, %bx   # buffer addr 0x7c00+512=0x7e00
    mov $_start_32, %bx  # buffer addr 0x7c00+512=0x7e00  
    mov $0x2,    %cx   # cl:start sect
    mov $0x0240, %ax   # ah-->0x02: read; al-->0x40=64: 64 nr of sects 64*512B
    mov $0x80,   %dx   # what's device 0x80 is the first disk 
    int $0x13          # read 64 sects from 2 sect to buffer addr 
    jc  read_self_all  # after read disk, check the cf to judge read success? if not jmp to continue read 


# 反汇编结果
	# mov $0x7e00, %bx   # buffer addr 0x7c00+512=0x7e00
    mov $_start_32, %bx  # buffer addr 0x7c00+512=0x7e00
    7c13:	bb 00 7e b9 02       	mov    $0x2b97e00,%ebx
```

## gdb x  /16x  0x7e00  打印内存值 ##



