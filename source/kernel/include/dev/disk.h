#ifndef DISK_H
#define DISK_H

#include "comm/types.h"

#define PART_NAME_SIZE              32      // 分区名称
#define DISK_NAME_SIZE              32      // 磁盘名称大小
#define DISK_CNT                    2       // 磁盘的数量
#define DISK_PRIMARY_PART_CNT       (4+1)       // 主分区数量最多才4个
#define DISK_PER_CHANNEL            2       // 每通道磁盘数量

// https://wiki.osdev.org/ATA_PIO_Mode#IDENTIFY_command
// 只考虑支持主总结primary bus
#define IOBASE_PRIMARY              0x1F0
#define	DISK_DATA(disk)				(disk->port_base + 0)		// 数据寄存器
#define	DISK_ERROR(disk)			(disk->port_base + 1)		// 错误寄存器
#define	DISK_SECTOR_COUNT(disk)		(disk->port_base + 2)		// 扇区数量寄存器
#define	DISK_LBA_LO(disk)			(disk->port_base + 3)		// LBA寄存器
#define	DISK_LBA_MID(disk)			(disk->port_base + 4)		// LBA寄存器
#define	DISK_LBA_HI(disk)			(disk->port_base + 5)		// LBA寄存器
#define	DISK_DRIVE(disk)			(disk->port_base + 6)		// 磁盘或磁头？
#define	DISK_STATUS(disk)			(disk->port_base + 7)		// 状态寄存器
#define	DISK_CMD(disk)				(disk->port_base + 7)		// 命令寄存器

// ATA命令
#define	DISK_CMD_IDENTIFY				0xEC	// IDENTIFY命令
#define	DISK_CMD_READ					0x24	// 读命令
#define	DISK_CMD_WRITE					0x34	// 写命令

// 状态寄存器
#define DISK_STATUS_ERR          (1 << 0)    // 发生了错误
#define DISK_STATUS_DRQ          (1 << 3)    // 准备好接受数据或者输出数据
#define DISK_STATUS_DF           (1 << 5)    // 驱动错误
#define DISK_STATUS_BUSY         (1 << 7)    // 正忙

#define	DISK_DRIVE_BASE		    0xE0		// 驱动器号基础值:0xA0 + LBA

struct _disk_t;

/**
 * @brief 分区类型
 */
typedef struct _partinfo_t {
    char name[PART_NAME_SIZE]; // 分区名称
    struct _disk_t * disk;      // 所属的磁盘

    // https://www.win.tue.nl/~aeb/partitions/partition_types-1.html
    enum {
        FS_INVALID = 0x00,      // 无效文件系统类型
        FS_FAT16_0 = 0x06,      // FAT16文件系统类型
        FS_FAT16_1 = 0x0E,
    }type;

	int start_sector;           // 起始扇区
	int total_sector;           // 总扇区
}partinfo_t;

/**
 * @brief 磁盘结构
 */
typedef struct _disk_t {
    char name[DISK_NAME_SIZE];      // 磁盘名称

    enum {
        DISK_DISK_MASTER = (0 << 4),     // 主设备
        DISK_DISK_SLAVE = (1 << 4),      // 从设备
    }drive;

    uint16_t port_base;             // 端口起始地址
    int sector_size;                // 块大小
    int sector_count;               // 总扇区数量
	partinfo_t partinfo[DISK_PRIMARY_PART_CNT];	// 分区表, 包含一个描述整个磁盘的假分区信息
}disk_t;

void disk_init (void);


#endif // DISK_H