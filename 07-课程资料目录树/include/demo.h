#ifndef CATALOG_DEMO_H
#define CATALOG_DEMO_H

#include "seqlist.h"

/*
 * 对照实验，对应题目里要求写进报告的几项比较：
 * 1. 顺序表与链表在表头插入时的移动次数；
 * 2. 同层文件的顺序查找、折半查找、分块查找、散列查找；
 * 3. 直接插入、折半插入、希尔、冒泡、快速排序的比较次数和移动次数。
 * level_files 是某一层已经收集好的文件；传 NULL 或空表时跳过“当前目录”那一节，
 * 仍然会用内置数据把五种排序和查找跑完。
 */
void demo_all_algorithms(const SeqList *level_files);

#endif
