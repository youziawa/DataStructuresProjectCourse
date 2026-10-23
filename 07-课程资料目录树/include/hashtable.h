#ifndef CATALOG_HASHTABLE_H
#define CATALOG_HASHTABLE_H

#include "common.h"
#include "seqlist.h"

/*
 * 链地址法散列表，按文件名查找同层文件。
 * bucket[i] 是第 i 个桶的链头。冲突时把新结点插到链头，时间 O(1)。
 * 同名文件允许共存：不同目录里可以有两份“习题.pdf”，它们落在同一条链上。
 * 散列函数是 djb2（h = h*33 + 字符），再对桶数取余。桶数由调用方指定，实验里用质数。
 *
 * 设元素个数 n、桶数 m、装填因子 α = n/m。
 * 查找成功的平均比较次数约为 1 + α/2（链地址、插在链头的常见估计），
 * 最坏是所有键落在同一个桶，退化成 O(n)。
 * 空间 O(n + m)，比顺序表多出桶数组和指针。
 */
typedef struct HashNode {
    FileInfo data;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode **bucket;
    int size;  /* 桶的个数 */
    int count; /* 已插入的记录数，含重名 */
} HashTable;

/* size < 1 时改用 211。成功返回 0。 */
int hash_init(HashTable *H, int size);
/* 释放所有链结点和桶数组。 */
void hash_destroy(HashTable *H);
/* 计算文件名对应的桶号，范围是 [0, size)。 */
unsigned hash_string(const char *s, int size);
/* 插入一条记录，允许同名。成功返回 0，内存不足返回 -1。 */
int hash_insert(HashTable *H, const FileInfo *e);
/*
 * 查找文件名。找到返回 0，并把第一条同名记录写入 out（out 可为 NULL）。
 * 找不到返回 -1。st->compares 是在该桶链上比较关键字的次数。
 */
int hash_search(const HashTable *H, const char *name, FileInfo *out, OpStat *st);
/*
 * 把该桶中全部同名记录追加进 out，返回本次命中个数。
 * out 必须已经 seq_init，函数不会先清空它。失败返回 -1。
 */
int hash_search_all(const HashTable *H, const char *name, SeqList *out, OpStat *st);

#endif
