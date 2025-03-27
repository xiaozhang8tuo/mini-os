ENTRY(_start)
SECTIONS
{
	. = 0x82000000;  /* 这里的偏移可以和shell loop冲突，因为是不同的进程，对应不同的页表，这个是相对偏移*/
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
		__bss_start__ = .;
		*(*.bss)
    	__bss_end__ = . ;
	}
}
