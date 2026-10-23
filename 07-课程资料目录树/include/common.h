#ifndef CATALOG_COMMON_H
#define CATALOG_COMMON_H

/*
 * 公共常量与记录类型。
 *
 * 题目只允许使用线性表、栈、队列、普通二叉树、图，以及查找、排序这些可选算法。
 * 本工程全部用 C 语言手写，不使用 C++ STL，也不调用库函数 qsort。
 * 本文件没有队列和图：题目 7 的目录树用不上它们，硬性要求是“不能超出范围”，不是“每种都要用”。
 */

#define MAX_NAME 128  /* 结点名或文件名的最大字节数，含结尾 '\0' 时实际可用 127 */
#define MAX_EXT 32    /* 扩展名最大字节数，不含点 */
#define MAX_PATH 256  /* 整条路径的最大字节数 */
#define MAX_DEPTH 32  /* 路径最多拆成多少层，防止异常输入把栈或数组撑爆 */

/*
 * 算法统计量。查找、排序、串匹配都会填写它，方便对照“比较次数 / 移动次数”，
 * 而不是只看墙钟时间（数据量小时计时没有区分度）。
 * compares：关键字比较或字符比较次数。
 * moves：元素赋值次数。顺序表移动元素、链表写入新结点都记在这里。
 */
typedef struct {
    long compares;
    long moves;
} OpStat;

/* 把统计量清零。s 允许为 NULL，调用方可以在不关心统计时传空指针。 */
static inline void stat_reset(OpStat *s) {
    if (s) {
        s->compares = 0;
        s->moves = 0;
    }
}

/*
 * 同层文件在顺序表、链表、散列表里的元素。
 * name 参与查找和排序；path 用来区分不同目录下的重名文件；
 * size 是按大小排序的关键字；ext 是按扩展名过滤时用的。
 */
typedef struct {
    char name[MAX_NAME];
    char path[MAX_PATH];
    long size;
    char ext[MAX_EXT];
} FileInfo;

#endif
