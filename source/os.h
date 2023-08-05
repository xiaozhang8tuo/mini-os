#ifndef __OS_H__
#define __OS_H__

#define KERNEL_CODE_SEG         (1 * 8)
#define KERNEL_DATA_SEG         (2 * 8)
#define APP_CODE_SEG            ((3 * 8) | 3)       // 特权级3
#define APP_DATA_SEG            ((4 * 8) | 3)       // 特权级3


#endif