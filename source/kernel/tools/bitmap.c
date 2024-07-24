#include "tools/bitmap.h"
#include "tools/klib.h"

int bitmap_byte_count (int bit_count) {
    return (bit_count + 8 - 1) / 8;         // 向上取整
}

void bitmap_init (bitmap_t * bitmap, uint8_t * bits, int count, int init_bit) {
    bitmap->bit_count = count;
    bitmap->bits = bits;

    int bytes = bitmap_byte_count(bitmap->bit_count);
    kernel_memset(bitmap->bits, init_bit ? 0xFF: 0, bytes);
}

int bitmap_get_bit(bitmap_t* bitmap, int index) {
    // 从低到高 17 / 8 = 2
    return bitmap->bits[index/8] & (1 << (index % 8));
}

void bitmap_set_bit(bitmap_t* bitmap, int index, int count, int bit){
    for (int i = 0; (i < count) && (index < bitmap->bit_count); i++, index++) {
        if (bit) {
            bitmap->bits[index/8] |= 1 << (index % 8);
        } else {
            bitmap->bits[index/8] &= ~(1 << (index % 8));
        }
    }
}

int bitmap_is_set(bitmap_t* bitmap, int index) {
    return bitmap_get_bit(bitmap, index) ? 1 : 0;
}

int bitmap_alloc_nbits (bitmap_t * bitmap, int bit, int count) {
    int search_idx = 0;
    int ok_idx = -1;

    while (search_idx < bitmap->bit_count) {
        // 定位到第一个相同的索引处
        if (bitmap_get_bit(bitmap, search_idx) != bit) {
            // 不同，继续寻找起始的bit
            search_idx++;
            continue;
        }

        // 记录起始索引
        ok_idx = search_idx;

        // 继续计算下一部分
        int i;
        for (i = 1; (i < count) && (search_idx < bitmap->bit_count); i++) {
            if (bitmap_get_bit(bitmap, search_idx++) != bit) {
                // 不足count个，退出，重新进行最外层的比较
                ok_idx = -1;
                break;
            }
        }

        // 找到，设置各位，然后退出
        if (i >= count) {
            bitmap_set_bit(bitmap, ok_idx, count, ~bit);
            return ok_idx;
        }
    }

    return -1;
}
