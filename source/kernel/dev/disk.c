#include "dev/disk.h"

static disk_t disk_buf[DISK_CNT];  // 通道结构

/**
 * @brief 磁盘初始化及检测
 * 以下只是将相关磁盘相关的信息给读取到内存中
 */
void disk_init (void) {
}