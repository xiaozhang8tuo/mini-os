#ifndef CONSOLE_H
#define CONSOLE_H
#include "comm/types.h"

// https://wiki.osdev.org/Printing_To_Screen
#define CONSOLE_DISP_ADDR           0xb8000
#define CONSOLE_DISP_END			(0xb8000 + 32*1024)	// 显存的结束地址
#define CONSOLE_ROW_MAX				25			// 行数
#define CONSOLE_COL_MAX				80			// 最大列数
// 显示器共80列，25行，按字符显示，每个字符需要用两个字节(VGA)模式下表示

/**
 * @brief 显示字符
 */
typedef struct _disp_char_t {
	uint16_t v;
}disp_char_t;

/**
 * 终端显示部件
 */
typedef struct _console_t {
	disp_char_t * disp_base;	// 显示基地址
    int display_rows, display_cols;	// 显示界面的行数和列数
}console_t;

int console_init (void);
int console_write (int dev, char * data, int size);
void console_close (int dev);

#endif /* SRC_UI_TTY_WIDGET_H_ */