ENTRY(_start) /* 指定入口地址 */
SECTIONS
{
    /* 为了方便调试，没有从0x80000000开始，以免与first_task冲突, 这个设置的值会体现在 phdr->p_vaddr 中; */
	. = 0x81000000;
	.text : {
		*(*.text)
	}

	.rodata : {
		*(*.rodata)
	}

	.data : {
		*(*.data)
	}

	.bss : {
		PROVIDE(__bss_start__ = .);
		*(*.bss)
    	__bss_end__ = . ;
	}
}