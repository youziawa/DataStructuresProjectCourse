/*
 * 把题目要求的对比直接跑出来：顺序表/链表的插入代价，五种排序的比较与移动，
 * 顺序、折半、分块、散列查找的比较次数。随机数用固定种子的线性同余，
 * 每次演示的数据相同，方便对照报告里的数字。
 */
#include "demo.h"

#include "hashtable.h"
#include "linklist.h"
#include "sortsearch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned rng = 20261018u;

/* 线性同余发生器。不调用 rand，生成过程固定，实验可以重复。 */
static int rnd(void) {
    rng = rng * 1103515245u + 12345u;
    return (int)((rng >> 16) & 32767);
}

static void print_files(const SeqList *L) {
    int i;
    for (i = 0; i < L->length; i++) {
        printf("  %s  大小=%ld\n", L->data[i].name, L->data[i].size);
    }
}

static void run_one_sort(const char *title, void (*fn)(SeqList *, SortKey, OpStat *),
                         const SeqList *src, SortKey key, int show) {
    SeqList tmp;
    OpStat st;
    seq_init(&tmp);
    seq_assign(&tmp, src);
    stat_reset(&st);
    fn(&tmp, key, &st);
    printf("%s：比较 %ld 次，移动 %ld 次，结果%s\n",
           title, st.compares, st.moves, is_sorted(&tmp, key) ? "有序" : "无序");
    if (show) {
        print_files(&tmp);
    }
    seq_destroy(&tmp);
}

static void fill_small(SeqList *L) {
    static const char *names[] = {"课件.pptx", "习题.pdf", "readme", "实验.pdf", "笔记.txt", "大纲.doc"};
    static const long sizes[] = {2048, 120, 10, 300, 80, 50};
    int i;
    seq_clear(L);
    for (i = 0; i < 6; i++) {
        FileInfo e;
        const char *dot = strrchr(names[i], '.');
        fileinfo_make(&e, names[i], names[i], sizes[i], dot ? dot + 1 : "");
        seq_push(L, &e);
    }
}

static void fill_random(SeqList *L, int n) {
    int i;
    int *order = (int *)malloc((size_t)n * sizeof(int));
    seq_clear(L);
    if (!order) {
        return;
    }
    for (i = 0; i < n; i++) {
        order[i] = i;
    }
    for (i = n - 1; i > 0; i--) {
        int j = rnd() % (i + 1);
        int t = order[i];
        order[i] = order[j];
        order[j] = t;
    }
    for (i = 0; i < n; i++) {
        FileInfo e;
        char name[MAX_NAME];
        snprintf(name, sizeof(name), "file%03d.txt", order[i]);
        fileinfo_make(&e, name, name, rnd() % 500, "txt");
        seq_push(L, &e);
    }
    free(order);
}

/* 两种表都在下标 0 连续插入。顺序表每次后移已有元素，链表只改指针。 */
static void bench_insert(int n) {
    SeqList seq;
    LinkList link;
    OpStat seq_st, link_st;
    int i;
    seq_init(&seq);
    link_init(&link);
    stat_reset(&seq_st);
    stat_reset(&link_st);
    for (i = 0; i < n; i++) {
        FileInfo e;
        char name[MAX_NAME];
        snprintf(name, sizeof(name), "n%03d", i);
        fileinfo_make(&e, name, name, i, "");
        seq_insert(&seq, 0, &e, &seq_st);
        link_insert_head(&link, &e, &link_st);
    }
    printf("在表头连续插入 %d 个同层文件记录：\n", n);
    printf("  顺序表赋值 %ld 次。公式 1+2+...+%d = %d。下标 0 处插入要后移已有元素。\n",
           seq_st.moves, n, n * (n + 1) / 2);
    printf("  链表赋值 %ld 次。头插只改指针，不移动其它元素。\n", link_st.moves);
    printf("  结论：频繁在表头插入时链表是 O(1)；需要按下标取中点做折半查找时必须用顺序表。\n");
    seq_destroy(&seq);
    link_destroy(&link);
}

/*
 * 在同一张已按文件名排序的表上比较查找。
 * 成功查找对每个关键字各查一次再取平均；失败查找用一个不存在的名字，顺序查找应比较 n 次。
 */
static void bench_search(const SeqList *sorted) {
    HashTable ht;
    LinkList link;
    OpStat st;
    int i;
    int mid;
    const char *hit;
    const char *miss = "不存在的文件";
    long seq_sum = 0, bin_sum = 0, hash_sum = 0;
    if (!sorted || sorted->length == 0) {
        return;
    }
    hash_init(&ht, 127);
    link_init(&link);
    for (i = 0; i < sorted->length; i++) {
        hash_insert(&ht, &sorted->data[i]);
        link_push_back(&link, &sorted->data[i], NULL);
    }
    for (i = 0; i < sorted->length; i++) {
        stat_reset(&st);
        seq_search(sorted, sorted->data[i].name, &st);
        seq_sum += st.compares;
        stat_reset(&st);
        bin_search(sorted, sorted->data[i].name, &st);
        bin_sum += st.compares;
        stat_reset(&st);
        hash_search(&ht, sorted->data[i].name, NULL, &st);
        hash_sum += st.compares;
    }
    mid = sorted->length / 2;
    hit = sorted->data[mid].name;
    printf("有序顺序表 n=%d，成功查找的平均比较次数：\n", sorted->length);
    printf("  顺序查找 %.2f（公式约 (n+1)/2 = %.2f），最坏 %d，空间 O(1)\n",
           (double)seq_sum / sorted->length, (sorted->length + 1) / 2.0, sorted->length);
    printf("  折半查找 %.2f（最坏 floor(log2 n)+1），空间 O(1)\n", (double)bin_sum / sorted->length);
    printf("  散列表 %.2f（链地址，桶数 %d，装填因子 %.2f；平均 O(1)，最坏 O(n)）\n",
           (double)hash_sum / sorted->length, ht.size, (double)sorted->length / ht.size);
    stat_reset(&st);
    printf("  查找“%s”：顺序 %d", hit, seq_search(sorted, hit, &st));
    printf("（比较 %ld）", st.compares);
    stat_reset(&st);
    printf("，折半 %d", bin_search(sorted, hit, &st));
    printf("（比较 %ld）", st.compares);
    stat_reset(&st);
    printf("，分块 %d", block_search(sorted, hit, 4, &st));
    printf("（比较 %ld）", st.compares);
    stat_reset(&st);
    hash_search(&ht, hit, NULL, &st);
    printf("，散列比较 %ld", st.compares);
    stat_reset(&st);
    printf("，链表 %d", link_search(&link, hit, &st));
    printf("（比较 %ld）\n", st.compares);
    stat_reset(&st);
    seq_search(sorted, miss, &st);
    printf("  查找不存在的“%s”：顺序比较 %ld 次（失败要扫完整表）\n", miss, st.compares);
    hash_destroy(&ht);
    link_destroy(&link);
}

static void search_level(const SeqList *level_files) {
    SeqList ordered;
    HashTable ht;
    int i;
    if (!level_files || level_files->length == 0) {
        printf("当前这一层没有文件，跳过同层检索。\n");
        return;
    }
    printf("同层文件 %d 个：\n", level_files->length);
    print_files(level_files);
    seq_init(&ordered);
    seq_assign(&ordered, level_files);
    insert_sort(&ordered, KEY_NAME, NULL);
    hash_init(&ht, 127);
    for (i = 0; i < level_files->length; i++) {
        OpStat st;
        stat_reset(&st);
        printf("  顺序查找“%s”下标=%d，比较 %ld 次\n",
               level_files->data[i].name,
               seq_search(level_files, level_files->data[i].name, &st),
               st.compares);
        hash_insert(&ht, &level_files->data[i]);
    }
    for (i = 0; i < ordered.length; i++) {
        OpStat st;
        stat_reset(&st);
        printf("  折半查找“%s”下标=%d，比较 %ld 次\n",
               ordered.data[i].name,
               bin_search(&ordered, ordered.data[i].name, &st),
               st.compares);
        stat_reset(&st);
        printf("  分块查找“%s”下标=%d，比较 %ld 次\n",
               ordered.data[i].name,
               block_search(&ordered, ordered.data[i].name, 2, &st),
               st.compares);
        stat_reset(&st);
        hash_search(&ht, ordered.data[i].name, NULL, &st);
        printf("  散列查找“%s”比较 %ld 次\n", ordered.data[i].name, st.compares);
    }
    hash_destroy(&ht);
    seq_destroy(&ordered);
}

void demo_all_algorithms(const SeqList *level_files) {
    SeqList small, random_list, sorted;
    rng = 20261018u;
    printf("\n========== 1. 顺序表和链表 ==========\n");
    bench_insert(100);
    printf("\n========== 2. 同层文件查找 ==========\n");
    search_level(level_files);
    printf("\n========== 3. 五种排序，同一组小数据，按文件名 ==========\n");
    printf("直接插入、折半插入最好 O(n)、最坏 O(n^2)；折半插入只减少比较，不减少移动。\n");
    printf("希尔增量 n/2，平均更好，最坏仍可能 O(n^2)。冒泡最好 O(n)、最坏 O(n^2)。\n");
    printf("快速排序平均 O(n log n)；枢轴取最左端时，已经有序的输入是 O(n^2)。\n");
    seq_init(&small);
    fill_small(&small);
    printf("排序前：\n");
    print_files(&small);
    run_one_sort("直接插入", insert_sort, &small, KEY_NAME, 1);
    run_one_sort("折半插入", bin_insert_sort, &small, KEY_NAME, 1);
    run_one_sort("希尔排序", shell_sort, &small, KEY_NAME, 1);
    run_one_sort("冒泡排序", bubble_sort, &small, KEY_NAME, 1);
    run_one_sort("快速排序", quick_sort, &small, KEY_NAME, 1);
    printf("\n同一组数据按大小排序：\n");
    run_one_sort("直接插入/大小", insert_sort, &small, KEY_SIZE, 1);
    run_one_sort("折半插入/大小", bin_insert_sort, &small, KEY_SIZE, 1);
    run_one_sort("希尔排序/大小", shell_sort, &small, KEY_SIZE, 1);
    run_one_sort("冒泡排序/大小", bubble_sort, &small, KEY_SIZE, 1);
    run_one_sort("快速排序/大小", quick_sort, &small, KEY_SIZE, 1);

    printf("\n========== 4. n=60 的比较次数（只看规模，不打印全部文件名） ==========\n");
    seq_init(&random_list);
    fill_random(&random_list, 60);
    run_one_sort("直接插入", insert_sort, &random_list, KEY_NAME, 0);
    run_one_sort("折半插入", bin_insert_sort, &random_list, KEY_NAME, 0);
    run_one_sort("希尔排序", shell_sort, &random_list, KEY_NAME, 0);
    run_one_sort("冒泡排序", bubble_sort, &random_list, KEY_NAME, 0);
    run_one_sort("快速排序", quick_sort, &random_list, KEY_NAME, 0);

    printf("\n========== 5. 查找性能 ==========\n");
    seq_init(&sorted);
    seq_assign(&sorted, &random_list);
    quick_sort(&sorted, KEY_NAME, NULL);
    bench_search(&sorted);
    printf("分块查找要先有序。块长 s、块数 b=n/s 时，平均比较大约 (b+s)/2，索引表是额外 O(b) 空间。\n");

    seq_destroy(&small);
    seq_destroy(&random_list);
    seq_destroy(&sorted);
}
