/*
 * 查找和排序都作用在顺序表上。链表只有顺序查找，放在 linklist.c。
 * 每种算法都把比较次数、移动次数记进 OpStat，st 可以传 NULL。
 * 相等关键字：直接插入、折半插入、冒泡保持原顺序；希尔和快速排序不保证稳定。
 */
#include "sortsearch.h"

#include <string.h>

int file_cmp(const FileInfo *a, const FileInfo *b, SortKey key) {
    if (key == KEY_SIZE) {
        if (a->size < b->size) {
            return -1;
        }
        if (a->size > b->size) {
            return 1;
        }
        return 0;
    }
    return strcmp(a->name, b->name);
}

int is_sorted(const SeqList *L, SortKey key) {
    int i;
    if (!L) {
        return 0;
    }
    for (i = 1; i < L->length; i++) {
        if (file_cmp(&L->data[i - 1], &L->data[i], key) > 0) {
            return 0;
        }
    }
    return 1;
}

int seq_search(const SeqList *L, const char *name, OpStat *st) {
    int i;
    if (!L || !name) {
        return -1;
    }
    for (i = 0; i < L->length; i++) {
        if (st) {
            st->compares++;
        }
        if (strcmp(L->data[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * 折半查找。区间是闭区间 [low, high]。
 * data[mid] 比目标小，目标只可能在右半边，low = mid+1；
 * 比目标大，目标只可能在左半边，high = mid-1。
 * is_sorted 的扫描不计入 st，避免把“检查有序”算进查找比较次数。
 */
int bin_search(const SeqList *L, const char *name, OpStat *st) {
    int low, high;
    if (!L || !name) {
        return -1;
    }
    if (!is_sorted(L, KEY_NAME)) {
        return -2;
    }
    low = 0;
    high = L->length - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int c;
        if (st) {
            st->compares++;
        }
        c = strcmp(L->data[mid].name, name);
        if (c == 0) {
            return mid;
        }
        if (c < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

/*
 * 分块查找。因为整表有序，第 i 块最大值就是 data[end]。
 * 第一个满足 name <= 块最大值的块才可能包含目标；进入该块后顺序查找，
 * 找不到就直接失败，不再看后面的块。
 */
int block_search(const SeqList *L, const char *name, int block_size, OpStat *st) {
    int n, b, i;
    if (!L || !name) {
        return -1;
    }
    if (!is_sorted(L, KEY_NAME)) {
        return -2;
    }
    if (block_size < 1) {
        block_size = 1;
    }
    n = L->length;
    if (n == 0) {
        return -1;
    }
    b = (n + block_size - 1) / block_size;
    for (i = 0; i < b; i++) {
        int start = i * block_size;
        int end = start + block_size - 1;
        int j;
        if (end >= n) {
            end = n - 1;
        }
        if (st) {
            st->compares++;
        }
        if (strcmp(name, L->data[end].name) <= 0) {
            for (j = start; j <= end; j++) {
                if (st) {
                    st->compares++;
                }
                if (strcmp(L->data[j].name, name) == 0) {
                    return j;
                }
            }
            return -1;
        }
    }
    return -1;
}

/* 直接插入。a[0..i-1] 已有序，把 a[i] 临时取出，大的元素向右移出空位。 */
void insert_sort(SeqList *L, SortKey key, OpStat *st) {
    int i;
    if (!L || L->length < 2) {
        return;
    }
    for (i = 1; i < L->length; i++) {
        FileInfo tmp = L->data[i];
        int j = i - 1;
        if (st) {
            st->moves++;
        }
        while (j >= 0) {
            if (st) {
                st->compares++;
            }
            if (file_cmp(&L->data[j], &tmp, key) <= 0) {
                break;
            }
            L->data[j + 1] = L->data[j];
            if (st) {
                st->moves++;
            }
            j--;
        }
        L->data[j + 1] = tmp;
        if (st) {
            st->moves++;
        }
    }
}

/*
 * 折半插入。循环结束时 low 就是插入位置。
 * 关键字相等走 else，low = mid+1，新元素插到相等元素后面，所以稳定。
 */
void bin_insert_sort(SeqList *L, SortKey key, OpStat *st) {
    int i;
    if (!L || L->length < 2) {
        return;
    }
    for (i = 1; i < L->length; i++) {
        FileInfo tmp = L->data[i];
        int low = 0;
        int high = i - 1;
        int j;
        if (st) {
            st->moves++;
        }
        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (st) {
                st->compares++;
            }
            if (file_cmp(&L->data[mid], &tmp, key) > 0) {
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
        for (j = i - 1; j >= low; j--) {
            L->data[j + 1] = L->data[j];
            if (st) {
                st->moves++;
            }
        }
        L->data[low] = tmp;
        if (st) {
            st->moves++;
        }
    }
}

/* 希尔排序。对每个 gap 做一趟分组插入，gap 最终变为 1，退化为直接插入。 */
void shell_sort(SeqList *L, SortKey key, OpStat *st) {
    int n, gap, i;
    if (!L || L->length < 2) {
        return;
    }
    n = L->length;
    for (gap = n / 2; gap > 0; gap /= 2) {
        for (i = gap; i < n; i++) {
            FileInfo tmp = L->data[i];
            int j = i;
            if (st) {
                st->moves++;
            }
            while (j >= gap) {
                if (st) {
                    st->compares++;
                }
                if (file_cmp(&L->data[j - gap], &tmp, key) <= 0) {
                    break;
                }
                L->data[j] = L->data[j - gap];
                if (st) {
                    st->moves++;
                }
                j -= gap;
            }
            L->data[j] = tmp;
            if (st) {
                st->moves++;
            }
        }
    }
}

/* 冒泡。第 i 趟之后，末尾 i 个元素已经就位，内层不用再看它们。swapped 为 0 说明已经有序。 */
void bubble_sort(SeqList *L, SortKey key, OpStat *st) {
    int i, n;
    if (!L || L->length < 2) {
        return;
    }
    n = L->length;
    for (i = 0; i < n - 1; i++) {
        int swapped = 0;
        int j;
        for (j = 0; j < n - 1 - i; j++) {
            if (st) {
                st->compares++;
            }
            if (file_cmp(&L->data[j], &L->data[j + 1], key) > 0) {
                FileInfo tmp = L->data[j];
                L->data[j] = L->data[j + 1];
                L->data[j + 1] = tmp;
                if (st) {
                    st->moves += 3;
                }
                swapped = 1;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

/*
 * 一趟划分。枢轴先存到 pivot，左端位置视为空位。
 * j 从右向左找比枢轴小的元素填到左边，i 从左向右找比枢轴大的元素填到右边。
 * i 与 j 相遇的位置就是枢轴的最终下标，其左边都不大于它，右边都不小于它。
 */
static void quick_rec(FileInfo *a, int left, int right, SortKey key, OpStat *st) {
    int i, j;
    FileInfo pivot;
    if (left >= right) {
        return;
    }
    pivot = a[left];
    if (st) {
        st->moves++;
    }
    i = left;
    j = right;
    while (i < j) {
        while (i < j) {
            if (st) {
                st->compares++;
            }
            if (file_cmp(&a[j], &pivot, key) < 0) {
                break;
            }
            j--;
        }
        if (i < j) {
            a[i] = a[j];
            if (st) {
                st->moves++;
            }
            i++;
        }
        while (i < j) {
            if (st) {
                st->compares++;
            }
            if (file_cmp(&a[i], &pivot, key) > 0) {
                break;
            }
            i++;
        }
        if (i < j) {
            a[j] = a[i];
            if (st) {
                st->moves++;
            }
            j--;
        }
    }
    a[i] = pivot;
    if (st) {
        st->moves++;
    }
    quick_rec(a, left, i - 1, key, st);
    quick_rec(a, i + 1, right, key, st);
}

void quick_sort(SeqList *L, SortKey key, OpStat *st) {
    if (!L || L->length < 2) {
        return;
    }
    quick_rec(L->data, 0, L->length - 1, key, st);
}
