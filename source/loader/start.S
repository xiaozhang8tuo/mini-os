/**
 * 二级引导，负责进行硬件检测，进入保护模式，然后加载内核，并跳转至内核运行
 * 
 */

  	# 16位代码，务必加上
  	.code16
 	.text
	.global _start
	.extern loader_entry
_start:
	jmp loader_entry

	.code32
	.text
	.global protect_mode_entry
	.extern load_kernel
protect_mode_entry:
	# 重新加载所有的数据段描述符
	mov $16, %ax		# 16为数据段选择子
	mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

	# 长跳转进入到32位内核加载模式中
	jmp $8, $load_kernel


