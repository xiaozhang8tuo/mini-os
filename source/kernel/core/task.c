#include "core/task.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "tools/log.h"
#include "cpu/mmu.h"
#include "core/memory.h"
#include "core/syscall.h"
#include "comm/elf.h"
#include "fs/fs.h"

static task_manager_t task_manager;
static uint32_t idle_task_stack[IDLE_TASK_STACK_SIZE];
static task_t task_table[TASK_NR];      // 用户进程表
static mutex_t task_table_mutex;        // 进程表互斥访问锁


// 初始化tss结构,设置入口地址,分配选择子等等
static int tss_init(task_t* task, int flag, uint32_t entry, uint32_t esp) {
    int tss_sel = gdt_alloc_desc();
    if (tss_sel < 0) {
        log_printf("alloc tss failed.\r\n");
        return -1;
    }

    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t),
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS
    );

    kernel_memset(&(task->tss), 0, sizeof(tss_t));

    // 分配内核栈，得到的是物理地址
    uint32_t kernel_stack = memory_alloc_page();
    if (kernel_stack == 0) {
        goto tss_init_failed;
    }

    int code_sel, data_sel;
    if (flag & TASK_FLAG_SYSTEM) {
        code_sel = KERNEL_SELECTOR_CS;
        data_sel = KERNEL_SELECTOR_DS;
    } else {
        // 注意加了RP3,不然将产生段保护错误
        code_sel = task_manager.app_code_sel | SEG_CPL3;
        data_sel = task_manager.app_data_sel | SEG_CPL3;
    }

    task->tss.eip = entry;
    task->tss.esp = esp ? esp : kernel_stack + MEM_PAGE_SIZE;  // 未指定栈则用内核栈，即运行在特权级0的进程
    task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE;
    task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.eflags = EFLAGS_DEFAULT| EFLAGS_IF;
    task->tss.es = task->tss.ss = task->tss.ds = task->tss.fs 
            = task->tss.gs = data_sel;   // 全部采用同一数据段
    task->tss.cs = code_sel; 
    task->tss.iomap = 0;

    // 页表初始化
    uint32_t page_dir = memory_create_uvm();
    if (page_dir == 0) {
        goto tss_init_failed;
    }
    task->tss.cr3 = page_dir;

    task->tss_sel = tss_sel;

    return 0;
tss_init_failed:
    gdt_free_sel(tss_sel);

    if (kernel_stack) {
        memory_free_page(kernel_stack);
    }
    return -1;
}

int task_init(task_t* task, const char* name, int flag, uint32_t entry, uint32_t esp) {
    ASSERT(task != (task_t *)0);

    tss_init(task, flag, entry, esp);
    // uint32_t* pesp = (uint32_t *)esp;
    // if (pesp) {
    //     *(--pesp) = entry; //这里填写任务的返回地址，恢复现场的时候ret用
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     task->stack = pesp;
    // }
    kernel_memcpy(task->name, (void*)name, TASK_NAME_SIZE);
    task->state = TASK_CREATED;
    task->time_ticks = TASK_TIME_SLICE_DEFAULT;
    task->sleep_ticks = 0;
    task->slice_ticks = task->time_ticks;
    task->parent = (task_t *)0;
    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);
    irq_state_t state = irq_enter_protection();
    task->pid = (uint32_t)task;   // 使用地址，能唯一
    task_set_ready(task);
    list_insert_last(&task_manager.task_list, &task->all_node);
    irq_leave_protection(state);
    return 0;
}


/**
 * @brief 任务任务初始时分配的各项资源
 */
void task_uninit (task_t * task) {
    // 选择子
    if (task->tss_sel) {
        gdt_free_sel(task->tss_sel);
    }

    // 栈空间
    if (task->tss.esp0) {
        memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE);
    }

    // 页表
    if (task->tss.cr3) {
        memory_destroy_uvm(task->tss.cr3);
    }

    kernel_memset(task, 0, sizeof(task_t));
}

void simple_switch(uint32_t** from, uint32_t* to);

void task_switch_from_to(task_t* from, task_t* to) {
    switch_to_tss(to->tss_sel);
    // simple_switch(&from->stack, to->stack);
}

static void idle_task_entry (void) {
    for (;;) {
        hlt();
    }
}

void task_manager_init(void) {
    kernel_memset(task_table, 0, sizeof(task_table));
    mutex_init(&task_table_mutex);

    int sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);
    task_manager.app_data_sel = sel;

    sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);
    task_manager.app_code_sel = sel;

    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);
    task_manager.curr_task = (task_t *)0;

    task_init(&task_manager.idle_task, "idle_task", TASK_FLAG_SYSTEM, 
                (uint32_t)idle_task_entry, 0);     // 运行于内核模式，无需指定特权级3的栈
}

void task_first_init(void) {
    void first_task_entry(void);
    extern uint8_t s_first_task[], e_first_task[];

    // 分配的空间比实际存储的空间要大一些，多余的用于放置栈
    uint32_t copy_size = (uint32_t)(e_first_task - s_first_task);
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE;
    ASSERT(copy_size < alloc_size);

    uint32_t first_start = (uint32_t)first_task_entry;

    task_init(&task_manager.first_task, "first task", 0, first_task_entry, first_start + alloc_size);
    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task = &task_manager.first_task;

    mmu_set_page_dir(task_manager.first_task.tss.cr3);

    // 分配一页内存供代码存放使用，然后将代码复制过去, PTE_U :用户态特权级3可以读
    memory_alloc_page_for(first_start,  alloc_size, PTE_P | PTE_W | PTE_U);
    kernel_memcpy((void *)first_start, (void *)s_first_task, copy_size);
}

task_t* task_first_task(void) {
    return &task_manager.first_task;
}


void task_set_ready(task_t* task) {
    if (task == &task_manager.idle_task) {
        return;
    }

    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}

void task_set_block(task_t* task) {
    if (task == &task_manager.idle_task) {
        return;
    }

    list_remove(&task_manager.ready_list, &task->run_node);
}

task_t * taks_next_run(void) {
    if (list_count(&task_manager.ready_list) == 0) {
        return &task_manager.idle_task;
    }

    list_node_t* task_node = list_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

task_t * task_current(void) {
    return task_manager.curr_task;
}

void task_dispatch(void) {
    // irq_state_t state = irq_enter_protection();
    task_t* to = taks_next_run();
    if (to != task_manager.curr_task) {
        task_t* from = task_current();
        task_manager.curr_task = to;
        to->state = TASK_RUNNING;

        task_switch_from_to(from, to);
    }
    // irq_leave_protection(state);
}

int sys_sched_yield(void) {
    irq_state_t state = irq_enter_protection();
    if (list_count(&task_manager.ready_list) > 1) {
        task_t* curr_task = task_current();

        task_set_block(curr_task);
        task_set_ready(curr_task);

        task_dispatch();
    }
    irq_leave_protection(state);
    return 0;
}

void task_timer_tick(void) {
    task_t* curr_task = task_current();
    if (--curr_task->slice_ticks == 0) {
        curr_task->slice_ticks = curr_task->time_ticks;
        
        task_set_block(curr_task);
        task_set_ready(curr_task);

        task_dispatch();
    }

    list_node_t* curr = list_first(&task_manager.sleep_list);
    while (curr)
    {
        list_node_t* next = list_node_next(curr);
        task_t* task = list_node_parent(curr, task_t, run_node);
        if (--task->sleep_ticks == 0) {
            task_set_wakeup(task);
            task_set_ready(task);
        }
        curr = next;
    }
    task_dispatch();
}

void task_set_sleep(task_t* task, uint32_t ticks) {
    if (ticks == 0) {
        return;
    }
    task->sleep_ticks = ticks;
    task->state = TASK_SLEEP;
    list_insert_last(&task_manager.sleep_list, &task->run_node);
}

void task_set_wakeup(task_t* task) {
    list_remove(&task_manager.sleep_list, &task->run_node);
}

void sys_sleep(uint32_t ms) {
    irq_state_t state = irq_enter_protection();
    task_set_block(task_manager.curr_task);
    task_set_sleep(task_manager.curr_task, (ms+OS_TICKS_MS-1)/OS_TICKS_MS);// 向上取整
    task_dispatch();
    irq_leave_protection(state);
}

/**
 * 返回任务的pid
 */
int sys_getpid (void) {
    task_t * curr_task = task_current();
    return curr_task->pid;
}

/**
 * 分配一个任务结构
 */
static task_t * alloc_task (void) {
    task_t * task = (task_t *)0;

    mutex_lock(&task_table_mutex);
    for (int i = 0; i < TASK_NR; i++) {
        task_t * curr = task_table + i;
        if (curr->name[0] == 0) {
            task = curr;
            break;
        }
    }
    mutex_unlock(&task_table_mutex);

    return task;
}

/**
 * 释放任务结构
 */
static void free_task (task_t * task) {
    mutex_lock(&task_table_mutex);
    task->name[0] = 0;
    mutex_unlock(&task_table_mutex);
}

/**
 * 创建进程的副本
 */
int sys_fork (void) {
    task_t * parent_task = task_current();

    // 分配任务结构
    task_t * child_task = alloc_task();
    if (child_task == (task_t *)0) {
        goto fork_failed;
    }
    syscall_frame_t * frame = (syscall_frame_t *)(parent_task->tss.esp0 - sizeof(syscall_frame_t));

    // 对子进程进行初始化, 并对必要的字段进行调整
    // 其中esp要减去系统调用的总参数字节大小，因为其是通过正常的ret返回, 而没有走系统调用处理的ret(参数个数返回)
    // 这里要注意子进程的esp不能直接设置成父进程的esp,要把传入的args这块pop出来. 子进程执行的位置,是父进程执行完毕lcall之后的位置
    int err = task_init(child_task,  parent_task->name, 0, frame->eip,
                        frame->esp + sizeof(uint32_t)*SYSCALL_PARAM_COUNT);
    if (err < 0) {
        goto fork_failed;
    }

    // 从父进程的栈中取部分状态，然后写入tss。
    // 注意检查esp, eip等是否在用户空间范围内，不然会造成page_fault
    tss_t * tss = &child_task->tss;
    tss->eax = 0;                       // 子进程返回0, 这就是为什么getpid返回值可以区分父子进程的原因
    tss->ebx = frame->ebx;
    tss->ecx = frame->ecx;
    tss->edx = frame->edx;
    tss->esi = frame->esi;
    tss->edi = frame->edi;
    tss->ebp = frame->ebp;

    tss->cs = frame->cs;
    tss->ds = frame->ds;
    tss->es = frame->es;
    tss->fs = frame->fs;
    tss->gs = frame->gs;
    tss->eflags = frame->eflags;

    child_task->parent = parent_task;
    // 复制父进程的内存空间到子进程
    if ((child_task->tss.cr3 = memory_copy_uvm(parent_task->tss.cr3)) < 0) {
        goto fork_failed;
    }

    // 创建成功，返回子进程的pid
    return child_task->pid;
fork_failed:
    if (child_task) {
        task_uninit (child_task);
        free_task(child_task);
    }
    return -1;
}

/**
 * @brief 加载一个程序表头的数据到内存中
 */
static int load_phdr(int file, Elf32_Phdr * phdr, uint32_t page_dir) {
    // 生成的ELF文件要求是页边界对齐的
    ASSERT((phdr->p_vaddr & (MEM_PAGE_SIZE - 1)) == 0);

    // 分配空间
    int err = memory_alloc_for_page_dir(page_dir, phdr->p_vaddr, phdr->p_memsz, PTE_P | PTE_U | PTE_W);
    if (err < 0) {
        log_printf("no memory");
        return -1;
    }

    // 调整当前的读写位置
    if (sys_lseek(file, phdr->p_offset, 0) < 0) {
        log_printf("read file failed");
        return -1;
    }

    // 为段分配所有的内存空间.后续操作如果失败，将在上层释放
    // 简单起见，设置成可写模式，也许可考虑根据phdr->flags设置成只读
    // 因为没有找到该值的详细定义，所以没有加上
    uint32_t vaddr = phdr->p_vaddr;
    uint32_t size = phdr->p_filesz;
    while (size > 0) {
        int curr_size = (size > MEM_PAGE_SIZE) ? MEM_PAGE_SIZE : size;

        uint32_t paddr = memory_get_paddr(page_dir, vaddr);

        // 注意，这里用的页表仍然是当前的
        if (sys_read(file, (char *)paddr, curr_size) <  curr_size) {
            log_printf("read file failed");
            return -1;
        }

        size -= curr_size;
        vaddr += curr_size;
    }

    // bss区考虑由crt0和cstart自行清0，这样更简单一些
    // 如果在上边进行处理，需要考虑到有可能的跨页表填充数据
    // 或者也可修改memory_alloc_for_page_dir，增加分配时清0页表，但这样开销较大
    // 所以，直接放在cstart哐crt0中直接内存填0，比较简单
    return 0;
}



/**
 * @brief 加载elf文件到内存中
 */
static uint32_t load_elf_file (task_t * task, const char * name, uint32_t page_dir) {
    Elf32_Ehdr elf_hdr;
    Elf32_Phdr elf_phdr;

    // 以只读方式打开
    int file = sys_open(name, 0);   // todo: flags暂时用0替代
    if (file < 0) {
        log_printf("open file failed.%s", name);
        goto load_failed;
    }

    // 先读取文件头
    int cnt = sys_read(file, (char *)&elf_hdr, sizeof(Elf32_Ehdr));
    if (cnt < sizeof(Elf32_Ehdr)) {
        log_printf("elf hdr too small. size=%d", cnt);
        goto load_failed;
    }

    // 做点必要性的检查。当然可以再做其它检查
    if ((elf_hdr.e_ident[0] != ELF_MAGIC) || (elf_hdr.e_ident[1] != 'E')
        || (elf_hdr.e_ident[2] != 'L') || (elf_hdr.e_ident[3] != 'F')) {
        log_printf("check elf indent failed.");
        goto load_failed;
    }

    // 必须是可执行文件和针对386处理器的类型，且有入口
    if ((elf_hdr.e_type != ET_EXEC) || (elf_hdr.e_machine != ET_386) || (elf_hdr.e_entry == 0)) {
        log_printf("check elf type or entry failed.");
        goto load_failed;
    }

    // 必须有程序头部
    if ((elf_hdr.e_phentsize == 0) || (elf_hdr.e_phoff == 0)) {
        log_printf("none programe header");
        goto load_failed;
    }

    // 然后从中加载程序头，将内容拷贝到相应的位置
    uint32_t e_phoff = elf_hdr.e_phoff;
    for (int i = 0; i < elf_hdr.e_phnum; i++, e_phoff += elf_hdr.e_phentsize) {
        if (sys_lseek(file, e_phoff, 0) < 0) {
            log_printf("read file failed");
            goto load_failed;
        }

        // 读取程序头后解析，这里不用读取到新进程的页表中，因为只是临时使用下
        cnt = sys_read(file, (char *)&elf_phdr, sizeof(Elf32_Phdr));
        if (cnt < sizeof(Elf32_Phdr)) {
            log_printf("read file failed");
            goto load_failed;
        }

        // 简单做一些检查，如有必要，可自行加更多
        // 主要判断是否是可加载的类型，并且要求加载的地址必须是用户空间
        if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr < MEMORY_TASK_BASE)) {
           continue;
        }

        // 加载当前程序头
        int err = load_phdr(file, &elf_phdr, page_dir);
        if (err < 0) {
            log_printf("load program hdr failed");
            goto load_failed;
        }
   }

    sys_close(file);
    return elf_hdr.e_entry;

load_failed:
    if (file >= 0) {
        sys_close(file);
    }
    return 0;
}


int sys_execve(char *name, char * const *argv, char * const *env) {
    task_t * task = task_current();

    // 现在开始加载了，先准备应用页表，由于所有操作均在内核区中进行，所以可以直接先切换到新页表
    uint32_t old_page_dir = task->tss.cr3;
    uint32_t new_page_dir = memory_create_uvm();
    if (!new_page_dir) {
        goto exec_failed;
    }

    // 加载elf文件到内存中。要放在开启新页表之后，这样才能对相应的内存区域写
    uint32_t entry = load_elf_file(task, name, new_page_dir);    // 暂时置用task->name表示
    if (entry == 0) {
        goto exec_failed;
    }

    // 准备用户栈空间，预留环境环境及参数的空间
    uint32_t stack_top = MEM_TASK_STACK_TOP;
    int err = memory_alloc_for_page_dir(
                            new_page_dir,
                            MEM_TASK_STACK_TOP - MEM_TASK_STACK_SIZE, //栈是从高到底的，
                            MEM_TASK_STACK_SIZE, 
                            PTE_P | PTE_U | PTE_W);
    if (err < 0) {
        goto exec_failed;
    }

    // 加载完毕，为程序的执行做必要准备
    // 注意，exec的作用是替换掉当前进程，所以只要改变当前进程的执行流即可
    // 当该进程恢复运行时，像完全重新运行一样，所以用户栈要设置成初始模式
    // 运行地址要设备成整个程序的入口地址
    syscall_frame_t * frame = (syscall_frame_t *)(task->tss.esp0 - sizeof(syscall_frame_t));
    frame->eip = entry;
    frame->eax = frame->ebx = frame->ecx = frame->edx = 0;
    frame->esi = frame->edi = frame->ebp = 0;
    frame->eflags = EFLAGS_DEFAULT| EFLAGS_IF;  // 段寄存器无需修改

    // 内核栈不用设置，保持不变，后面调用memory_destroy_uvm并不会销毁内核栈的映射。
    // 但用户栈需要更改, 同样要加上调用门的参数压栈空间
    frame->esp = stack_top - sizeof(uint32_t)*SYSCALL_PARAM_COUNT; // source/kernel/init/start.S retf $(5*4)

    // 切换到新的页表
    task->tss.cr3 = new_page_dir;
    mmu_set_page_dir(new_page_dir);   // 切换至新的页表。由于不用访问原栈及数据，所以并无问题

    // 调整页表，切换成新的，同时释放掉之前的
    // 当前使用的是内核栈，而内核栈并未映射到进程地址空间中，所以下面的释放没有问题
    memory_destroy_uvm(old_page_dir);            // 再释放掉了原进程的内容空间

    // 当从系统调用中返回时，将切换至新进程的入口地址运行，并且进程能够获取参数
    // 注意，如果用户栈设置不当，可能导致返回后运行出现异常。可在gdb中使用nexti单步观察运行流程
    return  0;

exec_failed:    // 必要的资源释放
    if (new_page_dir) {
        // 有页表空间切换，切换至旧页表，销毁新页表
        task->tss.cr3 = old_page_dir;
        mmu_set_page_dir(old_page_dir);
        memory_destroy_uvm(new_page_dir);
    }

    return -1;
}