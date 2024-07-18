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
