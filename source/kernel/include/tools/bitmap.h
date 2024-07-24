#ifndef BITMAP_H
#define BITMAP_H

#include "comm/types.h"

/**
 * @brief 位图数据结构
 */
typedef struct _bitmap_g {
    int bit_count;              // 位的数据
    uint8_t * bits;             // 位空间
}bitmap_t;

void bitmap_init (bitmap_t * bitmap, uint8_t * bits, int count, int init_bit);
int bitmap_byte_count (int bit_count);
int bitmap_get_bit(bitmap_t* bitmap, int index);
void bitmap_set_bit(bitmap_t* bitmap, int index, int count, int bit);
int bitmap_is_set(bitmap_t* bitmap, int index);
// 连续分配若干指定比特位，返回起始索引
int bitmap_alloc_nbits(bitmap_t* bitmap, int bit, int count);

#endif // BITMAP_H