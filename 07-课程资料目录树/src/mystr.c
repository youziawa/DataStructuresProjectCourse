/*
 * 串的运算。存储是堆上的连续字符数组，逻辑运算按教材中的串：赋值、比较、连接、
 * 求子串、朴素模式匹配。目录路径的拼接和文件名关键词查找都调用这里。
 */
#include "mystr.h"

#include <stdlib.h>
#include <string.h>

// 空串不分配 data，避免每个空串都 malloc(1)。
void StrInit(MyString *s) {
    s->data = NULL;
    s->length = 0;
}

// free(NULL) 是合法的，所以空串直接 free 也安全。
void StrDestroy(MyString *s) {
    if (!s) {
        return;
    }
    free(s->data);
    s->data = NULL;
    s->length = 0;
}

void StrClear(MyString *s) {
    StrDestroy(s);
}

const char *StrCStr(const MyString *s) {
    if (!s || !s->data) {
        return "";
    }
    return s->data;
}

int StrLength(const MyString *s) {
    return s ? s->length : 0;
}

// 先分配新空间，成功后再释放旧空间，分配失败时 s 保持原样。
int StrAssign(MyString *s, const char *cstr) {
    int n;
    char *nd;
    if (!s) {
        return -1;
    }
    if (!cstr) {
        cstr = "";
    }
    n = (int)strlen(cstr);
    nd = (char *)malloc((size_t)n + 1);
    if (!nd) {
        return -1;
    }
    memcpy(nd, cstr, (size_t)n + 1);
    free(s->data);
    s->data = nd;
    s->length = n;
    return 0;
}

int StrCopy(MyString *dest, const MyString *src) {
    if (!dest || !src) {
        return -1;
    }
    return StrAssign(dest, StrCStr(src));
}

// 逐字节比较。较短的串是较长串的前缀时，短串更小，用长度差表示。
int StrCompare(const MyString *a, const MyString *b) {
    int i = 0;
    int na = StrLength(a);
    int nb = StrLength(b);
    const char *da = StrCStr(a);
    const char *db = StrCStr(b);
    while (i < na && i < nb) {
        unsigned char ca = (unsigned char)da[i];
        unsigned char cb = (unsigned char)db[i];
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        i++;
    }
    return na - nb;
}

int StrAppend(MyString *dest, const MyString *src) {
    int n;
    char *nd;
    if (!dest || !src || src->length == 0) {
        return dest && src ? 0 : -1;
    }
    n = src->length;
    nd = (char *)realloc(dest->data, (size_t)dest->length + (size_t)n + 1);
    if (!nd) {
        return -1;
    }
    memcpy(nd + dest->length, src->data, (size_t)n);
    dest->length += n;
    nd[dest->length] = '\0';
    dest->data = nd;
    return 0;
}

int StrAppendCStr(MyString *dest, const char *cstr) {
    int n;
    char *nd;
    if (!dest) {
        return -1;
    }
    if (!cstr || cstr[0] == '\0') {
        return 0;
    }
    n = (int)strlen(cstr);
    nd = (char *)realloc(dest->data, (size_t)dest->length + (size_t)n + 1);
    if (!nd) {
        return -1;
    }
    memcpy(nd + dest->length, cstr, (size_t)n + 1);
    dest->length += n;
    dest->data = nd;
    return 0;
}

// 先连到临时串再交给 dest，这样 dest 与 a 或 b 重叠时不会读到已释放的内存。
int StrConcat(MyString *dest, const MyString *a, const MyString *b) {
    MyString tmp;
    if (!dest || !a || !b) {
        return -1;
    }
    StrInit(&tmp);
    if (StrAppend(&tmp, a) != 0 || StrAppend(&tmp, b) != 0) {
        StrDestroy(&tmp);
        return -1;
    }
    StrDestroy(dest);
    *dest = tmp;
    return 0;
}

// pos+len 允许等于 length，表示截到串尾；大于 length 则越界。
int SubString(MyString *sub, const MyString *s, int pos, int len) {
    char *nd;
    if (!sub || !s || pos < 0 || len < 0 || pos + len > s->length) {
        return -1;
    }
    nd = (char *)malloc((size_t)len + 1);
    if (!nd) {
        return -1;
    }
    if (len > 0) {
        memcpy(nd, s->data + pos, (size_t)len);
    }
    nd[len] = '\0';
    free(sub->data);
    sub->data = nd;
    sub->length = len;
    return 0;
}

/*
 * 朴素匹配：i 是主串对齐位置，j 是模式串下标。
 * 内层一旦失配就放弃这一趟，i 在外层加 1，j 从 0 重新开始。
 * 比较次数含成功匹配的那 p 个字符，也含失配的那一次。
 */
int StrIndex(const MyString *s, const MyString *pat, OpStat *st) {
    int i, j;
    int m, p;
    if (!s || !pat) {
        return -1;
    }
    if (pat->length == 0) {
        return 0;
    }
    m = s->length;
    p = pat->length;
    if (m < p) {
        return -1;
    }
    for (i = 0; i <= m - p; i++) {
        for (j = 0; j < p; j++) {
            if (st) {
                st->compares++;
            }
            if (s->data[i + j] != pat->data[j]) {
                break;
            }
        }
        if (j == p) {
            return i;
        }
    }
    return -1;
}
