#ifndef CATALOG_TESTS_H
#define CATALOG_TESTS_H

/*
 * 运行全部自测并打印 [PASS]/[FAIL]。
 * 覆盖空目录、单文件、多棵树组成的森林、同层重名、跨目录重名、路径不存在，
 * 以及串匹配、五种排序、四种查找、遍历序列和森林-二叉树可逆。
 * 返回失败个数，0 表示全部通过。
 */
int run_all_tests(void);

#endif
