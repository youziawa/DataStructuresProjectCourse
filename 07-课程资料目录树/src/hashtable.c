/*
 * 链地址法。插入总是放在桶的链头，所以后插入的同名文件会先被查到。
 * hash_search 返回第一条；hash_search_all 把整条链上的同名记录都收齐。
 */
#include "hashtable.h"

#include <stdlib.h>
#include <string.h>

int hash_init(HashTable *H, int size) {
    if (size < 1) {
        size = 211;
    }
    H->bucket = (HashNode **)calloc((size_t)size, sizeof(HashNode *));
    if (!H->bucket) {
        H->size = 0;
        H->count = 0;
        return -1;
    }
    H->size = size;
    H->count = 0;
    return 0;
}

void hash_destroy(HashTable *H) {
    int i;
    if (!H || !H->bucket) {
        return;
    }
    for (i = 0; i < H->size; i++) {
        HashNode *p = H->bucket[i];
        while (p) {
            HashNode *n = p->next;
            free(p);
            p = n;
        }
    }
    free(H->bucket);
    H->bucket = NULL;
    H->size = 0;
    H->count = 0;
}

// djb2：初值 5381，每读一个字节做 h = h*33 + c。用 unsigned long，溢出按模 2^n 回绕，是有定义的。
unsigned hash_string(const char *s, int size) {
    unsigned long h = 5381;
    const unsigned char *p = (const unsigned char *)(s ? s : "");
    if (size < 1) {
        return 0;
    }
    while (*p) {
        h = h * 33u + *p;
        p++;
    }
    return (unsigned)(h % (unsigned)size);
}

// 头插法处理冲突：新结点的 next 指向桶里原来的链，再让桶指向新结点。
int hash_insert(HashTable *H, const FileInfo *e) {
    unsigned idx;
    HashNode *node;
    if (!H || !H->bucket || !e) {
        return -1;
    }
    node = (HashNode *)malloc(sizeof(HashNode));
    if (!node) {
        return -1;
    }
    node->data = *e;
    idx = hash_string(e->name, H->size);
    node->next = H->bucket[idx];
    H->bucket[idx] = node;
    H->count++;
    return 0;
}

int hash_search(const HashTable *H, const char *name, FileInfo *out, OpStat *st) {
    unsigned idx;
    HashNode *p;
    if (!H || !H->bucket || !name) {
        return -1;
    }
    idx = hash_string(name, H->size);
    for (p = H->bucket[idx]; p; p = p->next) {
        if (st) {
            st->compares++;
        }
        if (strcmp(p->data.name, name) == 0) {
            if (out) {
                *out = p->data;
            }
            return 0;
        }
    }
    return -1;
}

int hash_search_all(const HashTable *H, const char *name, SeqList *out, OpStat *st) {
    unsigned idx;
    HashNode *p;
    int hits = 0;
    if (!H || !H->bucket || !name || !out) {
        return -1;
    }
    idx = hash_string(name, H->size);
    for (p = H->bucket[idx]; p; p = p->next) {
        if (st) {
            st->compares++;
        }
        if (strcmp(p->data.name, name) == 0) {
            if (seq_push(out, &p->data) != 0) {
                return -1;
            }
            hits++;
        }
    }
    return hits;
}
