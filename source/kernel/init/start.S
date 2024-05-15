    .text
    .extern kernel_init
    .global _start
    # void start (boot_info_t* boot_info)
_start:
    # 参照test是怎么取参数的
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax # 此时eax中的值，应该就是boot_info的地址，可以去loader_elf.txt 中查看boot_info的位置
    # 以上三句可替换为 mov 0x4(%esp), %eax   因为在进入函数的时候，esp指向的是返回地址，上面就是传入的参数

    # 参照是怎么，压栈参数，发起调用的
    # kernel_init(boot_info)
    push %eax
    call kernel_init
    jmp .