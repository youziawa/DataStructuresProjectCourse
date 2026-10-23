/*
 * 顺序表。容量从 8 起按 2 倍增长，扩容次数是 O(log n)，
 * 所以尾插的均摊代价是 O(1)。中间插入的元素移动仍然是 O(n)，扩容不能消除这一点。
 */
#include "seqlist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fileinfo_make(FileInfo *e, const char *name, const char *path, long size, const char *ext) {
    memset(e, 0, sizeof(*e));
    if (name) {
        snprintf(e->name, MAX_NAME, "%s", name);
    }
    if (path) {
        snprintf(e->path, MAX_PATH, "%s", path);
    }
    e->size = size;
    if (ext) {
        snprintf(e->ext, MAX_EXT, "%s", ext);
    }
}

int seq_init(SeqList *L) {
    L->data = NULL;
    L->length = 0;
    L->capacity = 0;
    return 0;
}

void seq_destroy(SeqList *L) {
    if (!L) {
        return;
    }
    free(L->data);
    L->data = NULL;
    L->length = 0;
    L->capacity = 0;
}

void seq_clear(SeqList *L) {
    if (L) {
        L->length = 0;
    }
}

int seq_reserve(SeqList *L, int need) {
    int cap;
    FileInfo *nd;
    if (need <= L->capacity) {
        return 0;
    }
    cap = L->capacity > 0 ? L->capacity : 8;
    while (cap < need) {
        cap *= 2;
    }
    nd = (FileInfo *)realloc(L->data, (size_t)cap * sizeof(FileInfo));
    if (!nd) {
        return -1;
    }
    L->data = nd;
    L->capacity = cap;
    return 0;
}

int seq_assign(SeqList *dest, const SeqList *src) {
    FileInfo *nd = NULL;
    if (src->length > 0) {
        nd = (FileInfo *)malloc((size_t)src->length * sizeof(FileInfo));
        if (!nd) {
            return -1;
        }
        memcpy(nd, src->data, (size_t)src->length * sizeof(FileInfo));
    }
    free(dest->data);
    dest->data = nd;
    dest->length = src->length;
    dest->capacity = src->length;
    return 0;
}

/* 从后往前搬，避免前面的元素把还没搬走的元素覆盖掉。 */
int seq_insert(SeqList *L, int index, const FileInfo *e, OpStat *st) {
    int i;
    if (!L || !e || index < 0 || index > L->length) {
        return -1;
    }
    if (seq_reserve(L, L->length + 1) != 0) {
        return -1;
    }
    for (i = L->length; i > index; i--) {
        L->data[i] = L->data[i - 1];
        if (st) {
            st->moves++;
        }
    }
    L->data[index] = *e;
    if (st) {
        st->moves++;
    }
    L->length++;
    return 0;
}

int seq_push(SeqList *L, const FileInfo *e) {
    return seq_insert(L, L->length, e, NULL);
}

int seq_get(const SeqList *L, int index, FileInfo *out) {
    if (!L || !out || index < 0 || index >= L->length) {
        return -1;
    }
    *out = L->data[index];
    return 0;
}

int seq_length(const SeqList *L) {
    return L ? L->length : 0;
}
