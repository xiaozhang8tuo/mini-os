__asm__(".code16gcc");

#include "loader.h"

static boot_info_t boot_info;

/**
 * BIOS下显示字符串
 */
static void show_msg(const char* msg){
    char c;

    // 使用bios写显存，持续往下写
    while( (c = *msg++) != '\0')
    {
        /*volitale 关键字使得汇编不做优化*/
        __asm__ __volatile__ (
            "mov $0xe, %%ah\n\t"
            "mov %[ch], %%al\n\t"
            "int $0x10"
            ::[ch]"r"(c)
        );
    }
}

static void detect_memory(void)
{
    uint32_t contID = 0;
	SMAP_entry_t smap_entry;
	int signature, bytes;

    show_msg("try to detect memory:");

    // 初次：EDX=0x534D4150,EAX=0xE820,ECX=24,INT 0x15, EBX=0（初次）
	// 后续：EAX=0xE820,ECX=24,
	// 结束判断：EBX=0
    boot_info.ram_region_count = 0;
    for (int i=0; i<BOOT_RAM_REGION_MAX; i++)
    {
        SMAP_entry_t * entry = &smap_entry;

        __asm__ __volatile__("int  $0x15"
			: "=a"(signature), "=c"(bytes), "=b"(contID)
			: "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150), "D"(entry));

        if (signature != 0x534D4150) {
            show_msg("failed.\r\n");
			return;
		}

		// todo: 20字节
		if (bytes > 20 && (entry->ACPI & 0x0001) == 0){
			continue;
		}

        // 保存RAM信息，只取32位，空间有限无需考虑更大容量的情况
        if (entry->Type == 1) {
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = entry->BaseL;
            boot_info.ram_region_cfg[boot_info.ram_region_count].size = entry->LengthL;
            boot_info.ram_region_count++;
        }

		if (contID == 0) {
			break;
		}

        show_msg("ok.\r\n");
    }

    // for (int i=0; i<boot_info.ram_region_count; i++)
    // {
    //     show_msg("")
    // }
}

// GDT表。临时用，后面内容会替换成自己的
uint16_t gdt_table[][4] = {
    {0, 0, 0, 0},
    {0xFFFF, 0x0000, 0x9A00, 0x00CF},
    {0xFFFF, 0x0000, 0x9200, 0x00CF},
};

/**
 * 进入保护模式
 */
static void  enter_protect_mode() {
    // 关中断
    cli();

    // 开启A20地址线，使得可访问1M以上空间
    // 使用的是Fast A20 Gate方式，见https://wiki.osdev.org/A20#Fast_A20_Gate
    uint8_t v = inb(0x92);
    outb(0x92, v | 0x2);

    // 加载GDT。由于中断已经关掉，IDT不需要加载
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));

    // 打开CR0的保护模式位，进入保持模式
    uint32_t cr0 = read_cr0();
    write_cr0(cr0 | (1 << 0));


    // 长跳转进入到保护模式
    // 使用长跳转，以便清空流水线，将里面的16位代码给清空
    far_jump(8, (uint32_t)protect_mode_entry);
}


void loader_entry(void)
{
    show_msg("...loading...\r\n");
    detect_memory();
    enter_protect_mode();
    for (;;){}
}