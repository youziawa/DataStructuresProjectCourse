#ifndef CATALOG_STACK_H
#define CATALOG_STACK_H

/*
 * 顺序栈，元素类型是 void*，用来压结点指针，不在栈里复制整棵树。
 * 空栈时 top == -1。capacity 不够时按 2 倍扩容。
 * 栈只保存指针，stack_destroy 不会释放指针指向的结点。
 * 非递归遍历、从文件回溯到根路径都用这个栈。
 * 入栈、出栈、读栈顶都是 O(1)；空间为当前栈深度，最坏 O(n)。
 */
typedef struct {
    void **data;
    int top;
    int capacity;
} SeqStack;

/* 置为空栈。 */
int stack_init(SeqStack *s);
/* 释放指针数组本身。 */
void stack_destroy(SeqStack *s);
/* 空栈返回 1，否则返回 0。 */
int stack_empty(const SeqStack *s);
/*
 * 入栈。成功返回 0，内存不足返回 -1。
 * 允许压入 NULL，但本工程约定不压空指针，这样 pop 返回 NULL 就能表示栈空。
 */
int stack_push(SeqStack *s, void *p);
/* 出栈。空栈返回 NULL。 */
void *stack_pop(SeqStack *s);
/* 读栈顶但不弹出。空栈返回 NULL。 */
void *stack_peek(const SeqStack *s);
/* 当前元素个数，空栈为 0。 */
int stack_size(const SeqStack *s);

#endif
