#ifndef CATALOG_SEQLIST_H
#define CATALOG_SEQLIST_H

#include "common.h"

/*
 * 顺序表。元素放在连续数组 data[0..length-1] 中。
 * 按下标访问、修改是 O(1)，所以折半查找和分块查找只能建在顺序表上。
 * 在下标 index 处插入要把后面的元素整体后移，最坏移动 O(n)。
 * capacity 是已分配容量，length 是当前元素个数。容量不够时按 2 倍扩容，
 * 扩容用 realloc，仍是顺序存储，不是链表。
 */
typedef struct {
    FileInfo *data;
    int length;
    int capacity;
} SeqList;

/*
 * 填写一条文件记录。超长的名字会被截断到 MAX_NAME-1，调用方应事先检查长度。
 * ext 传 NULL 表示没有扩展名。
 */
void fileinfo_make(FileInfo *e, const char *name, const char *path, long size, const char *ext);

// 初始化为空表。必须先调用，否则 data 是野指针。
int seq_init(SeqList *L);
// 释放数组。不释放 FileInfo 里面的指针，因为那些字段都是定长数组。
void seq_destroy(SeqList *L);
// 逻辑清空：length 置 0，容量保留，避免反复分配。
void seq_clear(SeqList *L);
// 保证容量至少为 need。成功返回 0，内存不足返回 -1。
int seq_reserve(SeqList *L, int need);
/*
 * dest 变成 src 的副本。dest 必须已经 seq_init。
 * 时间 O(n)，空间 O(n)。排序实验用它复制同一组数据，避免几种算法互相覆盖。
 */
int seq_assign(SeqList *dest, const SeqList *src);
// 在表尾追加。均摊 O(1)，因为偶尔才扩容。
int seq_push(SeqList *L, const FileInfo *e);
/*
 * 在 index 处插入，0 <= index <= length。index==length 就是追加。
 * 后面每个元素赋值一次，再加上写入新元素，所以移动次数是 (length-index)+1。
 * 在表头连续插入 n 次，总移动次数为 1+2+...+n = n(n+1)/2。
 * st 用来累计 moves。成功返回 0，下标非法或内存不足返回 -1。
 */
int seq_insert(SeqList *L, int index, const FileInfo *e, OpStat *st);
// 读取下标 index 的元素。越界返回 -1。
int seq_get(const SeqList *L, int index, FileInfo *out);
// 当前元素个数。
int seq_length(const SeqList *L);

#endif
