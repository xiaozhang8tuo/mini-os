	.text
	.global first_task_entry
	.extern first_task_main
first_task_entry:
	# 重新加载所有的数据段描述符
	mov %ss, %ax
	mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

	jmp first_task_main