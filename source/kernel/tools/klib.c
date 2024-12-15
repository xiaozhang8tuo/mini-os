#include "tools/klib.h"
#include "tools/log.h"
#include "comm/cpu_instr.h"

/**
 * @brief 计算字符串的数量
 */
int strings_count (char ** start) {
    int count = 0;

    if (start) {
        while (*start++) {
            count++;
        }
    }
    return count;
}


/**
 * @brief 从路径中解释文件名
 */
char * get_file_name (char * name) {
    char * s = name;

    // 定位到结束符
    while (*s != '\0') {
        s++;
    }

    // 反向搜索，直到找到反斜杆或者到文件开头
    while ((*s != '\\') && (*s != '/') && (s >= name)) {
        s--;
    }
    return s + 1;
}

void kernel_strcpy (char * dest, const char * src) {
    if (!dest || !src) {
        return;
    }

    while (*dest && *src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}


// strncpy 将 src 字符串最多 n 个字符复制到 dest 字符串中。
// 如果 src 的长度小于 n，则 dest 的剩余部分将用空字符（'\0'）填充。
// 如果 src 的长度大于或等于 n，则 dest 中不会自动添加终止符 '\0'。
void kernel_strncpy(char * dest, const char * src, int size) {
    if (!dest || !src || size <= 0) {
        return;
    }

    char * d = dest;
    const char * s = src;

    while (size-- > 0) {
        if ((*d++ = *s++) == '\0') {
            break;
        }
    }

    // Ensure the string is null-terminated only if there is space left
    if (size > 0) {
        *d = '\0';
    }
}

int kernel_strlen(const char * str) {
    if (str == (const char *)0) {
        return 0;
    }

	const char * c = str;

	int len = 0;
	while (*c++) {
		len++;
	}

	return len;
}

/**
 * 比较两个字符串，最多比较size个字符
 * 如果某一字符串提前比较完成，也算相同
 */
int kernel_strncmp (const char * s1, const char * s2, int size) {
    if (!s1 || !s2) {
        return -1;
    }

    while (*s1 && *s2 && (*s1 == *s2) && size) {
    	s1++;
    	s2++;
    }

    return !((*s1 == '\0') || (*s2 == '\0') || (*s1 == *s2));
}

void kernel_memcpy (void * dest, void * src, int size) {
    if (!dest || !src || !size) {
        return;
    }

    uint8_t * s = (uint8_t *)src;
    uint8_t * d = (uint8_t *)dest;
    while (size--) {
        *d++ = *s++;
    }
}

void kernel_memset(void * dest, uint8_t v, int size) {
    if (!dest || !size) {
        return;
    }

    uint8_t * d = (uint8_t *)dest;
    while (size--) {
        *d++ = v;
    }
}

int kernel_memcmp (void * d1, void * d2, int size) {
    if (!d1 || !d2) {
        return 1;
    }

	uint8_t * p_d1 = (uint8_t *)d1;
	uint8_t * p_d2 = (uint8_t *)d2;
	while (size--) {
		if (*p_d1++ != *p_d2++) {
			return 1;
		}
	}

	return 0;
}

void kernel_itoa(char* buf, int num, int base) {
    static const char* num2ch = "0123456789ABCDEF";
    char* p = buf;
    int old_num = num;

    if ((base != 2) && (base != 8) && (base != 10) && (base != 16)) {
        *p = '\0';
        return;
    }

    int signed_num = 0;
    if ((num < 0) && (base == 10)) {
        *p++ = '-';
        signed_num = 1;
    }

    if (signed_num) {
        do {
            char ch = num2ch[num % base];
            *p++ = ch;
            num /= base;
        } while (num);
    }
    else {
        uint32_t u_num = (uint32_t)num;
        do {
            char ch = num2ch[u_num % base];
            *p++ = ch;
            u_num /= base;
        } while (u_num);
    }
    *p-- = '\0';

    // 将转换结果逆序，生成最终的结果
    char* start = (!signed_num) ? buf : buf + 1;
    while (start < p) {
        char ch = *start;
        *start = *p;
        *p-- = ch;
        start++;
    }
}

void kernel_sprintf(char* str_buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kernel_vsprintf(str_buf, fmt, args);
    va_end(args);
}


void kernel_vsprintf(char* str_buf, const char* fmt, va_list args) {
    enum {NORMAL, READ_FMT} state = NORMAL;
    
    char* cur = str_buf;
    char ch;
    while((ch = *fmt++)) {
        switch (state)
        {
        case NORMAL:
            if (ch == '%')
            {
                state = READ_FMT;
            } else {
                *cur++ = ch;
            }
            break;
        case READ_FMT:
            if (ch == 'd') {
                int num = va_arg(args, int);
                kernel_itoa(cur, num, 10);
                cur += kernel_strlen(cur);
            } else if (ch == 'x') {
                int num = va_arg(args, int);
                kernel_itoa(cur, num, 16);
                cur += kernel_strlen(cur);
            } else if (ch == 'c') {
                char c = va_arg(args, int);
                *cur++ = c;
            } else if (ch == 's') {
                const char* str = va_arg(args, char*);
                int len = kernel_strlen(str);
                while(len--){
                    *cur++ = *str++;
                }
            }
            state = NORMAL;
            break;
        default:
            break;
        }
    }
}


void pannic (const char* file, int line, const char* func, const char* expr) {
    log_printf("ASSERT FAILED cond: %s", expr);
    log_printf("file: %s\r\nline: %d\r\nfunc: %s\r\n", file, line, func);
    for (;;)
        hlt();
}