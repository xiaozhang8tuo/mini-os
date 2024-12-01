#ifndef MEMORY_H
#define MEMORY_H
#include "comm/types.h"
#include "comm/boot_info.h"
#include "ipc/mutex.h"
#include "tools/bitmap.h"


#define MEM_EBDA_START 0x00080000
#define MEM_EXT_START (1024*1024)
#define MEM_PAGE_SIZE 4096
#define MEMORY_TASK_BASE (0x80000000)        // 进程起始线性地址空间
#define MEM_EXT_END (12*1024*1024 - 1)

#define MEM_TASK_STACK_TOP          (0xE0000000)        // 初始栈的位置  
#define MEM_TASK_STACK_SIZE         (MEM_PAGE_SIZE * 500)   // 初始500KB栈
#define MEM_TASK_ARG_SIZE           (MEM_PAGE_SIZE * 4)     // 参数和环境变量占用的大小


typedef struct _addr_alloc_t {
    mutex_t mutex;
    bitmap_t bitmap;

    uint32_t start;
    uint32_t size;
    uint32_t page_size;
}addr_alloc_t;

typedef struct _memory_map_t {
    void* vstart;
    void* vend;
    void* pstart;
    uint32_t perm;
}memory_map_t;


void memory_init(boot_info_t * boot_info);
uint32_t memory_create_uvm(void);

int memory_alloc_page_for (uint32_t addr, uint32_t size, int perm);

void memory_destroy_uvm (uint32_t page_dir);
uint32_t memory_copy_uvm (uint32_t page_dir);

uint32_t memory_get_paddr (uint32_t page_dir, uint32_t vaddr);

int memory_copy_uvm_data(uint32_t to, uint32_t page_dir, uint32_t from, uint32_t size);

#endif