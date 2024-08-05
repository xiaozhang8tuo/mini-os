#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/mmu.h"

static addr_alloc_t paddr_alloc;

static pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE))); // 内核页目录表

static void addr_alloc_init (addr_alloc_t * alloc, uint8_t * bits,
                    uint32_t start, uint32_t size, uint32_t page_size) {
    mutex_init(&alloc->mutex);
    alloc->start = start;
    alloc->size = size;
    alloc->page_size = page_size;
    bitmap_init(&alloc->bitmap, bits, alloc->size / page_size, 0);
}


/**
 * @brief 分配连续多页内存，填充bitmap，返回起始地址
 */
static uint32_t addr_alloc_page (addr_alloc_t * alloc, int page_count) {
    uint32_t addr = 0;

    mutex_lock(&alloc->mutex);

    int page_index = bitmap_alloc_nbits(&alloc->bitmap, 0, page_count);
    if (page_index >= 0) {
        addr = alloc->start + page_index * alloc->page_size;
    }

    mutex_unlock(&alloc->mutex);
    return addr;
}

/**
 * @brief 释放多页内存
 */
static void addr_free_page (addr_alloc_t * alloc, uint32_t addr, int page_count) {
    mutex_lock(&alloc->mutex);

    uint32_t pg_idx = (addr - alloc->start) / alloc->page_size;
    bitmap_set_bit(&alloc->bitmap, pg_idx, page_count, 0);

    mutex_unlock(&alloc->mutex);
}

void show_mem_info(boot_info_t * boot_info) {
    log_printf("mem region:");
    for (int i=0; i<boot_info->ram_region_count; i++) {
        log_printf("[%d]: 0x%x - 0x%x", i,
        boot_info->ram_region_cfg[i].start,
        boot_info->ram_region_cfg[i].size);
    }
    log_printf("\n");
}

static uint32_t total_mem_size (boot_info_t* boot_info) {
    uint32_t mem_size = 0;

    // 简单起见，暂不考虑中间有空洞的情况
    for (int i=0; i<boot_info->ram_region_count; i++) {
        mem_size += boot_info->ram_region_cfg[i].size;
    }
    return mem_size;
}

/**
 * @brief 根据虚拟地址找到对应的页表项，如果没有则创建新的页表
 */
pte_t * find_pte (pde_t * page_dir, uint32_t vaddr, int alloc) {
    pte_t * page_table;

    pde_t *pde = page_dir + pde_index(vaddr);
    if (pde->present) {
        page_table = (pte_t *)pde_paddr(pde);
    } else {
        if (alloc == 0) {
            return (pte_t *)0;
        }

        // 分配一个物理页表
        uint32_t pg_paddr = addr_alloc_page(&paddr_alloc, 1);
        if (pg_paddr == 0) {
            return (pte_t *)0;
        }

        // 设置为用户可读写，将被pte中设置所覆盖
        pde->v = pg_paddr | PTE_P;

        // 清空页表，防止出现异常
        // 这里虚拟地址和物理地址一一映射，所以直接写入
        page_table = (pte_t *)(pg_paddr);
        kernel_memset(page_table, 0, MEM_PAGE_SIZE);
    }

    return page_table + pte_index(vaddr);
}

/**
 * @brief 将指定的地址空间进行一页的映射
 */
int memory_create_map(pde_t* page_dir, uint32_t vaddr, uint32_t paddr, int count, uint32_t perm) {
    for (int i = 0; i < count; i++) {
        log_printf("create map: v-0x%x p-0x%x, perm: 0x%x", vaddr, paddr, perm);
        pte_t* pte = find_pte(page_dir, vaddr, 1);
        if (pte == (pte_t*)0) {
            return -1;
        }
        // 创建映射的时候，这条pte应当是不存在的。
        // 如果存在，说明可能有问题
        log_printf("\tpte addr: 0x%x", (uint32_t)pte);
        ASSERT(pte->present == 0);

        pte->v = paddr | perm | PTE_P;

        vaddr += MEM_PAGE_SIZE;
        paddr += MEM_PAGE_SIZE;
    }
}

/**
 * @brief 根据内存映射表，构造内核页表
 */
void create_kernel_table(void) {
    extern uint8_t s_text[], e_text[], s_data[];
    extern uint8_t kernel_base[];

    static memory_map_t kernel_map[] = {
        {kernel_base, s_text, kernel_base, 0},
        {s_text, e_text, s_text, 0},
        {s_data, (void*)MEM_EBDA_START, s_data, 0},
    };

    for (int i = 0; i < sizeof(kernel_map) / sizeof(memory_map_t); i++) {
        memory_map_t * map = kernel_map + i;

        // 可能有多个页，建立多个页的配置
        // 简化起见，不考虑4M的情况
        int vstart = down2((uint32_t)map->vstart, MEM_PAGE_SIZE);
        int vend = up2((uint32_t)map->vend, MEM_PAGE_SIZE);
        int page_count = (vend - vstart) / MEM_PAGE_SIZE;

        // 建立映射关系
        memory_create_map(kernel_page_dir, vstart, (uint32_t)map->pstart, page_count, map->perm);
    }
}

void memory_init(boot_info_t * boot_info) {
    // 1MB内存空间起始，在链接脚本中定义
    extern uint8_t* mem_free_start;
    log_printf("mem init");

    show_mem_info(boot_info);

    // 在内核数据后面放物理页位图
    uint8_t* mem_free = (uint8_t *)&mem_free_start;

    // 计算1MB以上空间的空闲内存容量，并对齐的页边界
    uint32_t mem_up1MB_free = total_mem_size(boot_info) - MEM_EXT_START;
    mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE);
    log_printf("free memory: 0x%x, size:0x%x", MEM_EXT_START, mem_up1MB_free);

    // 4GB大小需要总共4*1024*1024*1024/4096/8=128KB的位图, 使用低1MB的RAM空间中足够
    // 该部分的内存仅跟在 mem_free_start 开始放置
    addr_alloc_init(&paddr_alloc, mem_free, MEM_EXT_START, mem_up1MB_free, MEM_PAGE_SIZE);

    mem_free += bitmap_byte_count(paddr_alloc.size / MEM_PAGE_SIZE);

    ASSERT(mem_free < (uint8_t *)MEM_EBDA_START);

    create_kernel_table();
    mmu_set_page_dir((uint32_t)kernel_page_dir);
}

// void memory_init(boot_info_t * boot_info) {
//     addr_alloc_t addr_alloc;
//     uint8_t bits[8];

//     addr_alloc_init(&addr_alloc, bits, 0x1000, 64*4096, 4096);

//     for (int i = 0; i < 32; i++) {
//         uint32_t addr = addr_alloc_page(&addr_alloc, 2);
//         log_printf("alloc addr: 0x%x", addr);
//     }

//     uint32_t addr = 0x1000;
//     for (int i = 0; i < 32; i++) {
//         addr_free_page(&addr_alloc, addr, 2);
//         addr += 8192;
//     }
// }