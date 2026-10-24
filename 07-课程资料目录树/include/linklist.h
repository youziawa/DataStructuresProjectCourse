#ifndef CATALOG_LINKLIST_H
#define CATALOG_LINKLIST_H

#include "common.h"

/*
 * 带头结点、带尾指针的单链表。
 * head 是哨兵，不存放文件；真正的数据从 head->next 开始。
 * tail 指向最后一个数据结点，空表时 tail == head，因此尾插不用扫描整表，是 O(1)。
 * 头插也是 O(1)，不移动其它元素。这是它和顺序表在“同层文件插入”上的差别。
 * 不能 O(1) 按下标取中间结点，所以折半查找不在链表上做。
 */
typedef struct LNode {
    FileInfo data;
    struct LNode *next;
} LNode;

typedef struct {
    LNode *head; // 头结点，data 字段无意义
    LNode *tail; // 最后一个数据结点；空表时等于 head
    int length;
} LinkList;

// 创建头结点。失败返回 -1。
int link_init(LinkList *L);
// 释放全部结点，包括头结点。
void link_destroy(LinkList *L);
// 删除全部数据结点，保留头结点。
void link_clear(LinkList *L);
/*
 * 头插。新结点插在头结点之后，成为第一个数据。
 * 时间 O(1)。st->moves 记 1，表示只给新结点赋值，不搬移旧元素。
 */
int link_insert_head(LinkList *L, const FileInfo *e, OpStat *st);
/*
 * 尾插，保持和顺序表相同的先后次序。时间 O(1)。
 * 收集某一层文件时用尾插，这样链表和顺序表的第 i 个元素是同一个文件。
 */
int link_push_back(LinkList *L, const FileInfo *e, OpStat *st);
/*
 * 按文件名顺序查找。找到返回从 0 开始的序号，找不到返回 -1。
 * 成功平均比较 (n+1)/2 次，失败比较 n 次。空间 O(1)。
 */
int link_search(const LinkList *L, const char *name, OpStat *st);
// 取第 index 个数据结点。需要沿指针走 index 步，时间 O(index)。
int link_get(const LinkList *L, int index, FileInfo *out);
// 数据结点个数，不含头结点。
int link_length(const LinkList *L);

#endif
