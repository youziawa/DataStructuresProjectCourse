/*
 * 顺序栈。data[0] 是栈底，data[top] 是栈顶。
 * 遍历和路径回溯只把已有结点的地址压进来，出栈后结点仍由森林负责释放。
 */
#include "stack.h"

#include <stdlib.h>

int stack_init(SeqStack *s) {
    s->data = NULL;
    s->top = -1;
    s->capacity = 0;
    return 0;
}

void stack_destroy(SeqStack *s) {
    if (!s) {
        return;
    }
    free(s->data);
    s->data = NULL;
    s->top = -1;
    s->capacity = 0;
}

int stack_empty(const SeqStack *s) {
    return !s || s->top < 0;
}

int stack_size(const SeqStack *s) {
    return s ? s->top + 1 : 0;
}

// 先扩容再写入。realloc 失败时栈内容不变，调用方看到 -1。
int stack_push(SeqStack *s, void *p) {
    if (s->top + 1 >= s->capacity) {
        int cap = s->capacity > 0 ? s->capacity * 2 : 16;
        void **nd = (void **)realloc(s->data, (size_t)cap * sizeof(void *));
        if (!nd) {
            return -1;
        }
        s->data = nd;
        s->capacity = cap;
    }
    s->data[++s->top] = p;
    return 0;
}

void *stack_pop(SeqStack *s) {
    if (stack_empty(s)) {
        return NULL;
    }
    return s->data[s->top--];
}

void *stack_peek(const SeqStack *s) {
    if (stack_empty(s)) {
        return NULL;
    }
    return s->data[s->top];
}
