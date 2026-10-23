/*
 * 单链表。头结点让“插在第一个位置”和“插在中间”不用分别判断 head 是否为空。
 * 尾指针让尾插保持 O(1)。查找仍然必须沿 next 扫描，时间 O(n)。
 */
#include "linklist.h"

#include <stdlib.h>
#include <string.h>

int link_init(LinkList *L) {
    L->head = (LNode *)malloc(sizeof(LNode));
    if (!L->head) {
        return -1;
    }
    memset(&L->head->data, 0, sizeof(FileInfo));
    L->head->next = NULL;
    L->tail = L->head;
    L->length = 0;
    return 0;
}

void link_clear(LinkList *L) {
    LNode *p;
    if (!L || !L->head) {
        return;
    }
    p = L->head->next;
    while (p) {
        LNode *n = p->next;
        free(p);
        p = n;
    }
    L->head->next = NULL;
    L->tail = L->head;
    L->length = 0;
}

void link_destroy(LinkList *L) {
    if (!L) {
        return;
    }
    link_clear(L);
    free(L->head);
    L->head = NULL;
    L->tail = NULL;
    L->length = 0;
}

/* 新结点的 next 先指向原第一个数据，再让头结点指向新结点。空表时还要更新 tail。 */
int link_insert_head(LinkList *L, const FileInfo *e, OpStat *st) {
    LNode *node;
    if (!L || !L->head || !e) {
        return -1;
    }
    node = (LNode *)malloc(sizeof(LNode));
    if (!node) {
        return -1;
    }
    node->data = *e;
    node->next = L->head->next;
    L->head->next = node;
    if (L->tail == L->head) {
        L->tail = node;
    }
    L->length++;
    if (st) {
        st->moves++;
    }
    return 0;
}

/* tail 一直指向最后结点，新结点直接接在 tail 后面，再把 tail 移过去。 */
int link_push_back(LinkList *L, const FileInfo *e, OpStat *st) {
    LNode *node;
    if (!L || !L->head || !e) {
        return -1;
    }
    node = (LNode *)malloc(sizeof(LNode));
    if (!node) {
        return -1;
    }
    node->data = *e;
    node->next = NULL;
    L->tail->next = node;
    L->tail = node;
    L->length++;
    if (st) {
        st->moves++;
    }
    return 0;
}

int link_search(const LinkList *L, const char *name, OpStat *st) {
    LNode *p;
    int index = 0;
    if (!L || !L->head || !name) {
        return -1;
    }
    for (p = L->head->next; p; p = p->next, index++) {
        if (st) {
            st->compares++;
        }
        if (strcmp(p->data.name, name) == 0) {
            return index;
        }
    }
    return -1;
}

int link_get(const LinkList *L, int index, FileInfo *out) {
    LNode *p;
    int i;
    if (!L || !out || index < 0 || index >= L->length) {
        return -1;
    }
    p = L->head->next;
    for (i = 0; i < index; i++) {
        p = p->next;
    }
    *out = p->data;
    return 0;
}

int link_length(const LinkList *L) {
    return L ? L->length : 0;
}
