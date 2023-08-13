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


