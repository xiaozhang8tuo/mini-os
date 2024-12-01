#ifndef KLIB_H
#define KLIB_H
#include "comm/types.h"
#include <stdarg.h>

static inline uint32_t down2(uint32_t size, uint32_t bound) {
    // return size / bound * bound;
    return size & ~(bound - 1);
}

static inline uint32_t up2(uint32_t size, uint32_t bound) {
    return (size + bound - 1) & ~(bound - 1);
}

int strings_count (char ** start);
char * get_file_name (char * name);


void kernel_strcpy (char * dest, const char * src);
void kernel_strncpy(char * dest, const char * src, int size);
int kernel_strncmp (const char * s1, const char * s2, int size);
int kernel_strlen(const char * str);

void kernel_memcpy (void * dest, void * src, int size);
void kernel_memset(void * dest, uint8_t v, int size);
int kernel_memcmp (void * d1, void * d2, int size);

void kernel_sprintf(char* str_buf, const char* fmt, ...);
void kernel_vsprintf(char* str_buf, const char* fmt, va_list args);

#ifndef RELEASE

#define ASSERT(expr)    \
        if (!(expr))    \
            pannic(__FILE__, __LINE__, __func__, #expr)

void pannic (const char* file, int line, const char* func, const char* expr);
#else
#define ASSERT(expr) ((void)0)
#endif


#endif