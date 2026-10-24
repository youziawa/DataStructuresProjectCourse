#ifndef CATALOG_MYSTR_H
#define CATALOG_MYSTR_H

#include "common.h"

/*
 * 堆分配串（动态顺序串）。
 * 路径拼接、求子串、朴素模式匹配都使用本类型，避免只用 char[] 却没有串运算。
 * data 以 '\0' 结尾，length 不含这个结尾符。空串时 data 可以是 NULL，length 为 0。
 * 下标从 0 开始，和 C 字符数组一致。
 */
typedef struct {
    char *data;
    int length;
} MyString;

// 初始化为空串。调用其它运算之前必须先初始化，否则 data 是野指针。
void StrInit(MyString *s);
// 释放堆空间并置空。可以重复调用。
void StrDestroy(MyString *s);
// 清空内容，效果与销毁后再初始化相同。
void StrClear(MyString *s);

/*
 * 用 C 字符串给 s 赋值，会释放 s 原来的空间。
 * cstr 为 NULL 时当成空串。成功返回 0，内存不足返回 -1。
 * 时间 O(n)，n 为字符数；空间 O(n)。
 */
int StrAssign(MyString *s, const char *cstr);
// 复制 src 到 dest。成功返回 0，失败返回 -1。
int StrCopy(MyString *dest, const MyString *src);
/*
 * 按字典序比较。返回值小于 0、等于 0、大于 0 分别表示 a<b、a==b、a>b。
 * 比较的是无符号字节，这样中文 UTF-8 也不会因符号位被看成负数。时间 O(min(na, nb))。
 */
int StrCompare(const MyString *a, const MyString *b);
// 返回长度。s 为空指针时返回 0。
int StrLength(const MyString *s);
/*
 * dest = a 接上 b。dest 可以和 a、b 是同一个变量，内部会先写到临时串。
 * 时间 O(|a|+|b|)，空间 O(|a|+|b|)。成功返回 0。
 */
int StrConcat(MyString *dest, const MyString *a, const MyString *b);
// 把 src 追加到 dest 末尾。时间 O(|src|)，可能重新分配 dest。
int StrAppend(MyString *dest, const MyString *src);
// 把 C 字符串追加到 dest。路径拼接“目录/文件名”用这个函数。
int StrAppendCStr(MyString *dest, const char *cstr);
/*
 * 求子串。pos 从 0 计，len 是要截取的长度。
 * 越界返回 -1，成功时 sub 被替换成新子串。时间 O(len)，空间 O(len)。
 */
int SubString(MyString *sub, const MyString *s, int pos, int len);
/*
 * 朴素模式匹配（Brute Force）。
 * 从每个对齐位置起逐个比较字符，失配后模式串回到起点，主串只前进 1 位。
 * 返回第一次出现的下标；找不到返回 -1；空模式串按约定返回 0。
 * 设主串长 m、模式长 p：最坏时间 O(m*p)，例如主串全是 a、模式是 a...ab；
 * 一般文本的平均比较次数接近 O(m+p)。额外空间 O(1)。
 * st 不为空时，compares 记录字符比较次数。
 */
int StrIndex(const MyString *s, const MyString *pat, OpStat *st);
// 取出 C 字符串指针，便于 printf。空串返回指向常量 "" 的指针，不会返回 NULL。
const char *StrCStr(const MyString *s);

#endif
