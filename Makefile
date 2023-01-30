UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	TOOL_PREFIX = x86_64-linux-gnu-
else
	TOOL_PREFIX = x86_64-elf-
endif

# GCC parm
#	m32 				来创建 32 位可执行文件和共享库
#	fno-pie 			该标志告诉 gcc 不要创建位置无关的可执行文件(PIE)。
#	fno-stack-protector 编译任何未链接到标准用户空间库
#	nostdlib 			不连接系统标准启动文件和标准库文件，只把指定的文件传递给连接器。这个选项常用于编译内核、bootloader等程序，它们不需要启动文件、标准库文件。
# 	nostdinc 			不搜索默认路径头文件

CFLAGS = -g -c -O0 -m32 -fno-pie -fno-stack-protector -nostdlib -nostdinc

# ld parm
# -m elf_i386 64位机成功链接32位目标文件
# -Ttext=0x7c00 指定起始位置 -Tbss=org/-Tdata=org/-Ttext=org Same as --section-start, with ".bss", ".data" or ".text" as the sectionname.
# http://www.ruanyifeng.com/blog/2015/09/0x7c00.html 为什么主引导记录的内存地址是0x7C00？

all: source/os.c source/os.h source/start.s
	$(TOOL_PREFIX)gcc $(CFLAGS) source/start.S
	$(TOOL_PREFIX)gcc $(CFLAGS) source/os.c	

	$(TOOL_PREFIX)ld -m elf_i386 -Ttext=0x7c00 start.o os.o -o os.elf
	${TOOL_PREFIX}objcopy -O binary os.elf os.bin
	${TOOL_PREFIX}objdump -x -d -S  os.elf > os_dis.txt	
	${TOOL_PREFIX}readelf -a  os.elf > os_elf.txt
	dd if=os.bin of=image/disk.img conv=notrunc

clean:
	rm -f *.elf *.o *.bin os_dis.txt os_elf.txt start.s
