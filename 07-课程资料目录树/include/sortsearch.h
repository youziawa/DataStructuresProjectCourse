#ifndef CATALOG_SORTSEARCH_H
#define CATALOG_SORTSEARCH_H

#include "seqlist.h"

// 排序关键字。同层文件可以按文件名，也可以按大小。
typedef enum {
    KEY_NAME = 0,
    KEY_SIZE = 1
} SortKey;

/*
 * 比较两条记录。
 * KEY_NAME：按文件名字节序，和 strcmp 一致。
 * KEY_SIZE：只比大小。大小相等时返回 0，这样稳定排序能保留原来的先后次序。
 * 返回值 <0、=0、>0 分别表示 a 应排在 b 前面、两者关键字相等、a 应排在 b 后面。
 */
int file_cmp(const FileInfo *a, const FileInfo *b, SortKey key);
// 是否已按 key 非降序排好。空表和单元素表视为有序。
int is_sorted(const SeqList *L, SortKey key);

/*
 * 顺序查找。从表头扫到表尾，找到返回下标，找不到返回 -1。
 * 成功时平均比较 (n+1)/2 次，最坏 n 次；失败一定比较 n 次。额外空间 O(1)。
 * 不要求表有序。
 */
int seq_search(const SeqList *L, const char *name, OpStat *st);
/*
 * 折半查找，只比较文件名，所以表必须先按文件名升序。
 * 未排序返回 -2，避免在无序表上给出错误下标。
 * 每次取中间下标 mid = low + (high-low)/2，写成这样是为了避免 low+high 溢出。
 * 最坏比较次数 floor(log2 n)+1，平均大约 log2(n+1)-1。判定树高度是对数，空间 O(1)。
 * 链表不能 O(1) 取中点，所以折半查找只实现在顺序表上。
 */
int bin_search(const SeqList *L, const char *name, OpStat *st);
/*
 * 分块查找。表必须按文件名升序。块长 block_size，最后一块可以不满。
 * 索引不单独建表：第 i 块的最大键就是该块最后一个元素，因为表已经有序。
 * 先顺序查索引，找到 name 可能落入的块，再在块内顺序查找。
 * 设块数 b = ceil(n/s)，平均比较约 (b+s)/2。s 取 sqrt(n) 时平均约为 O(sqrt(n))。
 * 未排序返回 -2，空表或找不到返回 -1。
 */
int block_search(const SeqList *L, const char *name, int block_size, OpStat *st);

/*
 * 直接插入排序。把 a[i] 插入到已经有序的 a[0..i-1] 中，从右向左找位置并后移。
 * 最好：已经有序，每趟只比较 1 次就停，时间 O(n)，移动 2(n-1) 次（取出再放回）。
 * 平均和最坏：逆序时比较和移动都是 O(n^2)。空间 O(1)。关键字相等时不交换，稳定。
 */
void insert_sort(SeqList *L, SortKey key, OpStat *st);
/*
 * 折半插入排序。查找插入位置用折半，所以比较次数降到约 O(n log n)；
 * 找到位置后仍要把一段元素后移，移动次数和直接插入一样，最坏仍是 O(n^2)。
 * 空间 O(1)。相等关键字插到已有相等元素的后面，因此稳定。
 */
void bin_insert_sort(SeqList *L, SortKey key, OpStat *st);
/*
 * 希尔排序。增量序列是 n/2, n/4, ... , 1，最后一趟增量一定是 1，所以结果有序。
 * 增量较大时移动的是相距 gap 的元素，比直接插入更早把逆序对拉开。
 * 本增量序列的最坏情况仍可能达到 O(n^2)，平均通常好于 O(n^2)。空间 O(1)。
 * 分组插入跨越了相等元素，不稳定。
 */
void shell_sort(SeqList *L, SortKey key, OpStat *st);
/*
 * 冒泡排序。相邻元素逆序就交换，一趟把当前最大元素沉到末尾。
 * 某一趟没有发生交换就提前结束：已经有序时只比较 n-1 次，时间 O(n)。
 * 逆序时比较 n(n-1)/2 次，时间 O(n^2)。空间 O(1)。只在大于时交换，稳定。
 */
void bubble_sort(SeqList *L, SortKey key, OpStat *st);
/*
 * 快速排序。枢轴取当前区间最左端，用双向扫描划分：右边比枢轴小的换到左边，
 * 左边比枢轴大的换到右边，最后枢轴落到最终位置，再递归左右两段。
 * 平均时间 O(n log n)，平均递归深度 O(log n)。
 * 枢轴取左端时，输入已经有序会使划分极度不均匀，最坏时间 O(n^2)，递归深度 O(n)。
 * 不稳定。
 */
void quick_sort(SeqList *L, SortKey key, OpStat *st);

#endif
